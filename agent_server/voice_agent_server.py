
#!/usr/bin/env python3
"""
Voice Agent gRPC 테스트 서버 (grpc.aio) - 지연 측정 및 Signaling 등록

gRPC:
  - GetAudioFormats : 오디오 포맷 안내
  - AudioStream     : AudioFrame 양방향 스트리밍

모드:
  echo : 받은 프레임을 그대로 돌려보냄
  tone : 20ms 주기로 사인파 프레임 송신

Signaling:
  - WebSocket으로 Signaling Server의 /agent에 연결
  - {"cmd": "regist", "endpoint": "IP:PORT"} 전송
  - regist_res 응답 수신
  - 연결이 끊어지면 재접속 및 재등록

Endpoint:
  --endpoint를 지정하면 해당 IP/도메인을 사용
  생략하면 Signaling Server로 나가는 로컬 IP를 자동 탐색
  Signaling Server 주소는 기본적으로 ws://<machine-ip>:8080/agent 사용
  gRPC listen 포트는 --port (기본값 50051) 사용

설치:
  pip install grpcio grpcio-tools websockets

실행 예:
  python voice_agent_server_2.py
  python voice_agent_server_2.py --signaling-ip 192.168.0.100
  python voice_agent_server_2.py --signaling-ip 192.168.0.100 --signaling-port 8080

  python voice_agent_server_2.py --port 10101 --mode echo --no-stamp \
      --signaling-url ws://192.168.0.100:8080/agent

  python agent_server.py --port 50051 --mode echo \
      --signaling-url wss://signal.example.com/agent \
      --endpoint agent.example.com
"""

import argparse
import asyncio
import gc
import json
import logging
import math
import signal
import socket
import struct
import time

from typing import List
from urllib.parse import urlsplit

import grpc
import websockets

import voice_agent_pb2 as pb
import voice_agent_pb2_grpc as pb_grpc


log = logging.getLogger("voice_agent")


# ----------------------------------------------------------------------
# Audio configuration
# ----------------------------------------------------------------------

SAMPLE_RATE_HZ = 16000
CHANNELS = 1
FRAME_MS = 20
SAMPLES_PER_FRAME = SAMPLE_RATE_HZ * FRAME_MS // 1000
OUT_QUEUE_MAX = 100

STAMP_LEN = 12
STAMP_FMT = "<Iq"

MAX_SAMPLES = 200_000
MAX_PLAUSIBLE_NS = 10 * 1_000_000_000


# ----------------------------------------------------------------------
# Endpoint
# ----------------------------------------------------------------------

def detect_machine_ip() -> str:
    """기본 라우트에서 사용하는 머신의 IPv4 주소를 탐색한다.

    UDP connect는 실제 패킷을 전송하지 않고 OS 라우팅 테이블로
    선택된 로컬 주소만 확인한다. 네트워크가 없는 환경에서는
    hostname에 등록된 non-loopback IPv4를 fallback으로 사용한다.
    """

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.connect(("8.8.8.8", 53))
        local_ip = sock.getsockname()[0]
        if local_ip and not local_ip.startswith("127."):
            return local_ip
    except OSError:
        pass
    finally:
        sock.close()

    try:
        addresses = socket.getaddrinfo(
            socket.gethostname(), None, family=socket.AF_INET,
            type=socket.SOCK_DGRAM,
        )
        for address in addresses:
            local_ip = address[4][0]
            if local_ip and not local_ip.startswith("127."):
                return local_ip
    except OSError:
        pass

    return "127.0.0.1"


def detect_local_ip(signaling_url: str) -> str:
    """
    Signaling Server 방향으로 통신할 때 사용할 로컬 IP를 탐색한다.

    UDP connect는 실제 데이터를 전송하지 않는다.
    OS의 라우팅 테이블을 이용해 출발지 IP를 확인한다.
    """

    parsed = urlsplit(signaling_url)

    if parsed.scheme not in ("ws", "wss") or not parsed.hostname:
        raise ValueError(f"Invalid signaling URL: {signaling_url}")

    signaling_host = parsed.hostname
    signaling_port = parsed.port

    if signaling_port is None:
        signaling_port = 443 if parsed.scheme == "wss" else 80

    addresses = socket.getaddrinfo(
        signaling_host,
        signaling_port,
        type=socket.SOCK_DGRAM,
    )

    last_error = None

    for family, socktype, proto, _, sockaddr in addresses:
        sock = socket.socket(family, socktype, proto)

        try:
            sock.connect(sockaddr)
            local_ip = sock.getsockname()[0]

            if local_ip:
                return local_ip

        except OSError as e:
            last_error = e

        finally:
            sock.close()

    raise RuntimeError(
        f"Cannot detect local IP for {signaling_host}: {last_error}"
    )


def make_endpoint(endpoint_host: str, port: int) -> str:
    """
    IP 또는 도메인에 gRPC 포트를 결합한다.

    IPv6 주소는 [IPv6]:PORT 형태로 구성한다.
    """

    host = endpoint_host.strip()

    if not host:
        raise ValueError("Endpoint host is empty")

    # IPv6 주소를 대괄호로 감싸서 전달한 경우 정리
    if host.startswith("[") and host.endswith("]"):
        host = host[1:-1]

    # 이미 포트까지 입력한 경우 잘못된 설정으로 처리
    if host.count(":") == 1:
        raise ValueError(
            "--endpoint에는 포트를 입력하지 마세요. "
            "포트는 --port 값이 자동으로 추가됩니다."
        )

    if ":" in host:
        return f"[{host}]:{port}"

    return f"{host}:{port}"


def resolve_endpoint(
    signaling_url: str,
    endpoint_host: str,
    port: int,
) -> str:

    if endpoint_host:
        host = endpoint_host
        log.info("Using configured endpoint host: %s", host)
    else:
        host = detect_local_ip(signaling_url)
        log.info("Detected local endpoint IP: %s", host)

    return make_endpoint(host, port)


# ----------------------------------------------------------------------
# Queue helper
# ----------------------------------------------------------------------

def _offer(q: asyncio.Queue, item) -> bool:
    """큐가 가득 차면 가장 오래된 항목을 버린다."""

    dropped = False

    while True:
        try:
            q.put_nowait(item)
            return dropped

        except asyncio.QueueFull:
            try:
                q.get_nowait()
                dropped = True
            except asyncio.QueueEmpty:
                pass


# ----------------------------------------------------------------------
# Statistics
# ----------------------------------------------------------------------

class Samples:

    def __init__(self, name: str):
        self.name = name
        self.v: List[float] = []

    def add(self, us: float) -> None:
        if len(self.v) < MAX_SAMPLES:
            self.v.append(us)

    def summary(self) -> str:
        if not self.v:
            return f"{self.name}: no samples"

        s = sorted(self.v)
        n = len(s)

        def pct(p: float) -> float:
            return s[min(n - 1, int(p * n))]

        avg = sum(s) / n

        return (
            f"{self.name}(us) n={n} min={s[0]:.0f} "
            f"avg={avg:.0f} p50={pct(0.50):.0f} "
            f"p95={pct(0.95):.0f} p99={pct(0.99):.0f} "
            f"max={s[-1]:.0f}"
        )


# ----------------------------------------------------------------------
# gRPC service
# ----------------------------------------------------------------------

class VoiceAgentService(pb_grpc.VoiceAgentServiceServicer):

    def __init__(self, mode: str, stamp: bool, warmup: int):
        self._mode = mode
        self._stamp = stamp
        self._warmup = warmup
        self._stream_seq = 0

    async def GetAudioFormats(self, request, context):

        fmt = pb.AudioFormat(
            is_encoding=False,
            sample_rate_hz=SAMPLE_RATE_HZ,
            channels=CHANNELS,
        )

        log.info("GetAudioFormats from %s", context.peer())

        return pb.AudioFormatReply(
            ingress_formats=[fmt],
            egress_formats=[fmt],
        )

    async def AudioStream(self, request_iterator, context):

        self._stream_seq += 1
        sid = self._stream_seq

        log.info(
            "[%d] stream open peer=%s mode=%s stamp=%s",
            sid, context.peer(), self._mode, self._stamp
        )

        out_q: asyncio.Queue = asyncio.Queue(maxsize=OUT_QUEUE_MAX)

        stats = {
            "rx": 0,
            "rx_bytes": 0,
            "tx": 0,
            "drop": 0,
            "invalid": 0,
        }

        owd_in = Samples("gateway->agent")
        dwell = Samples("server_dwell ")

        async def reader():

            try:
                async for frame in request_iterator:

                    t_recv = time.monotonic_ns()
                    data = frame.pcm_s16le

                    stats["rx"] += 1
                    stats["rx_bytes"] += len(data)

                    if self._mode != "echo":
                        continue

                    if self._stamp:

                        if len(data) >= STAMP_LEN:

                            seq, t_send = struct.unpack_from(
                                STAMP_FMT, data, 0
                            )

                            delta = t_recv - t_send

                            if 0 <= delta < MAX_PLAUSIBLE_NS:

                                if seq >= self._warmup:
                                    owd_in.add(delta / 1000.0)

                            else:
                                stats["invalid"] += 1

                        else:
                            stats["invalid"] += 1

                    if _offer(out_q, (data, t_recv)):
                        stats["drop"] += 1

                log.info("[%d] client half-closed", sid)

            except asyncio.CancelledError:
                raise

            except Exception as e:
                log.info("[%d] reader ended: %r", sid, e)

            finally:
                _offer(out_q, None)

        async def tone_producer():

            phase = 0.0
            step = 2.0 * math.pi * 440.0 / SAMPLE_RATE_HZ
            next_t = time.monotonic()

            while True:

                samples = []

                for _ in range(SAMPLES_PER_FRAME):
                    samples.append(int(8000 * math.sin(phase)))
                    phase += step

                if phase > 2.0 * math.pi:
                    phase -= 2.0 * math.pi

                data = struct.pack(
                    "<%dh" % SAMPLES_PER_FRAME,
                    *samples
                )

                if _offer(out_q, (data, None)):
                    stats["drop"] += 1

                next_t += FRAME_MS / 1000.0

                await asyncio.sleep(
                    max(0.0, next_t - time.monotonic())
                )

        tasks = [asyncio.create_task(reader())]

        if self._mode == "tone":
            tasks.append(asyncio.create_task(tone_producer()))

        try:

            while True:

                item = await out_q.get()

                if item is None:
                    break

                data, t_recv = item

                if self._stamp and t_recv is not None:

                    t_tx = time.monotonic_ns()

                    if len(data) >= STAMP_LEN:

                        seq = struct.unpack_from("<I", data, 0)[0]

                        buf = bytearray(data)

                        struct.pack_into("<q", buf, 4, t_tx)

                        data = bytes(buf)

                        if seq >= self._warmup:
                            dwell.add((t_tx - t_recv) / 1000.0)

                stats["tx"] += 1

                yield pb.AudioFrame(pcm_s16le=data)

        except asyncio.CancelledError:

            log.info("[%d] cancelled by peer", sid)
            raise

        finally:

            for t in tasks:
                t.cancel()

            await asyncio.gather(*tasks, return_exceptions=True)

            log.info(
                "[%d] stream closed rx=%d (%d bytes) tx=%d "
                "drop=%d invalid_stamp=%d",
                sid,
                stats["rx"],
                stats["rx_bytes"],
                stats["tx"],
                stats["drop"],
                stats["invalid"],
            )

            if self._mode == "echo" and self._stamp:
                log.info("[%d] %s", sid, owd_in.summary())
                log.info("[%d] %s", sid, dwell.summary())


# ----------------------------------------------------------------------
# Signaling WebSocket client
# ----------------------------------------------------------------------

async def signaling_client(
    signaling_url: str,
    endpoint: str,
    stop: asyncio.Event,
) -> None:
    """
    Signaling Server에 Agent를 등록한다.

    연결이 끊어지면 재접속하고 다시 등록한다.
    """

    while not stop.is_set():

        try:

            log.info("Connecting to Signaling Server: %s", signaling_url)

            async with websockets.connect(
                signaling_url,
                open_timeout=10,
                close_timeout=5,
                ping_interval=20,
                ping_timeout=20,
            ) as ws:

                log.info("Signaling WebSocket connected")

                regist_msg = {
                    "cmd": "regist",
                    "endpoint": endpoint,
                }

                await ws.send(json.dumps(regist_msg))

                log.info("Agent registration sent: %s", endpoint)

                # 등록 응답 대기
                response_text = await asyncio.wait_for(
                    ws.recv(),
                    timeout=10,
                )

                response = json.loads(response_text)

                if response.get("cmd") != "regist_res":
                    raise RuntimeError(
                        f"Unexpected registration response: {response}"
                    )

                if response.get("result") != "success":
                    raise RuntimeError(
                        f"Agent registration failed: {response}"
                    )

                agent_id = response.get("id", "")

                log.info(
                    "Agent registered successfully: id=%s endpoint=%s",
                    agent_id,
                    endpoint,
                )

                # 연결 유지 및 향후 서버 메시지 수신
                async for message in ws:
                    log.debug("Signaling message received: %s", message)

                log.warning("Signaling WebSocket disconnected")

        except asyncio.CancelledError:
            raise

        except Exception as e:
            log.warning("Signaling connection error: %s", e)

        if not stop.is_set():

            log.info("Retrying Signaling connection in 3 seconds...")

            try:
                await asyncio.wait_for(stop.wait(), timeout=3)

            except asyncio.TimeoutError:
                pass


# ----------------------------------------------------------------------
# Server
# ----------------------------------------------------------------------

async def serve(
    host: str,
    port: int,
    mode: str,
    stamp: bool,
    warmup: int,
    signaling_url: str,
    endpoint_host: str,
) -> None:

    server = grpc.aio.server(
        options=[
            ("grpc.keepalive_permit_without_calls", 1),
            ("grpc.http2.min_recv_ping_interval_without_data_ms", 5000),
            ("grpc.http2.max_ping_strikes", 0),
            ("grpc.keepalive_time_ms", 20000),
            ("grpc.keepalive_timeout_ms", 5000),
            ("grpc.max_receive_message_length", 1 * 1024 * 1024),
        ]
    )

    pb_grpc.add_VoiceAgentServiceServicer_to_server(
        VoiceAgentService(mode, stamp, warmup),
        server,
    )

    addr = f"{host}:{port}"

    if server.add_insecure_port(addr) == 0:
        raise RuntimeError(f"Failed to bind gRPC address: {addr}")

    await server.start()

    log.info(
        "gRPC listening on %s (mode=%s, stamp=%s, %dHz/%dch/%dms)",
        addr,
        mode,
        stamp,
        SAMPLE_RATE_HZ,
        CHANNELS,
        FRAME_MS,
    )

    stop = asyncio.Event()
    signaling_task = None

    loop = asyncio.get_running_loop()

    for sig in (signal.SIGINT, signal.SIGTERM):
        loop.add_signal_handler(sig, stop.set)

    try:

        if signaling_url:

            endpoint = resolve_endpoint(
                signaling_url,
                endpoint_host,
                port,
            )

            log.info("Agent endpoint: %s", endpoint)

            signaling_task = asyncio.create_task(
                signaling_client(
                    signaling_url,
                    endpoint,
                    stop,
                )
            )

        await stop.wait()

    finally:

        log.info("Shutting down...")

        if signaling_task is not None:
            signaling_task.cancel()
            await asyncio.gather(
                signaling_task,
                return_exceptions=True,
            )

        await server.stop(grace=3)

        log.info("Server stopped")


# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------

def main() -> None:

    ap = argparse.ArgumentParser()

    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=50051)

    ap.add_argument("--mode", choices=["echo", "tone"], default="echo")

    ap.add_argument(
        "--warmup",
        type=int,
        default=10,
        help="통계에서 제외할 앞쪽 seq 개수",
    )

    ap.add_argument(
        "--stamp",
        dest="stamp",
        action="store_true",
        help="스탬프 파싱/재스탬프 비활성화",
    )

    ap.add_argument(
        "--gc-off",
        action="store_true",
        help="GC 비활성화",
    )

    ap.add_argument(
        "--signaling-url",
        default="",
        help="Signaling Server WebSocket URL (지정하면 --signaling-ip/--signaling-port보다 우선)",
    )

    ap.add_argument(
        "--signaling-ip",
        default="",
        help="Signaling Server IP (기본값: 머신의 로컬 IPv4)",
    )

    ap.add_argument(
        "--signaling-port",
        type=int,
        default=8080,
        help="Signaling Server port (기본값: 8080)",
    )

    ap.add_argument(
        "--endpoint",
        default="",
        help="Gateway에서 접근할 Agent IP 또는 도메인. 포트 제외",
    )

    ap.add_argument("-v", "--verbose", action="store_true")

    ap.set_defaults(stamp=False)

    args = ap.parse_args()

    if args.port < 1 or args.port > 65535:
        ap.error("--port must be between 1 and 65535")

    if args.signaling_port < 1 or args.signaling_port > 65535:
        ap.error("--signaling-port must be between 1 and 65535")

    signaling_url = args.signaling_url
    if not signaling_url:
        signaling_ip = args.signaling_ip or detect_machine_ip()
        signaling_url = f"ws://{signaling_ip}:{args.signaling_port}/agent"

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
    )

    if args.gc_off:
        gc.disable()
        log.warning("gc disabled (experiment only)")

    asyncio.run(
        serve(
            args.host,
            args.port,
            args.mode,
            args.stamp,
            args.warmup,
            signaling_url,
            args.endpoint,
        )
    )


if __name__ == "__main__":
    main()
