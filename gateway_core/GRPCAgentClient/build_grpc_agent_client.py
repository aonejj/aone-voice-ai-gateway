#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys


def run(cmd):
    print("[grpc_agent_client] " + " ".join(cmd), flush=True)
    subprocess.check_call(cmd)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--src-dir", required=True) 
    p.add_argument("--build-dir", required=True)  
    p.add_argument("--lib-out-dir", required=True) 
    p.add_argument("--build-type", required=True) 
    p.add_argument("--output", required=True) 
    a = p.parse_args()

    os.makedirs(a.build_dir, exist_ok=True)
    try:
        run(["cmake", "-S", a.src_dir, "-B", a.build_dir,
             "-DCMAKE_BUILD_TYPE=" + a.build_type,
             "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" + os.path.abspath(a.lib_out_dir)])
        run(["cmake", "--build", a.build_dir, "-j", str(os.cpu_count() or 4)])
    except subprocess.CalledProcessError as e:
        print("[grpc_agent_client] build failed (exit %d)" % e.returncode, file=sys.stderr)
        return e.returncode or 1
    except FileNotFoundError:
        print("[grpc_agent_client] cmake not found in PATH", file=sys.stderr)
        return 1

    if not os.path.exists(a.output):
        print("[grpc_agent_client] expected output not found: " + a.output, file=sys.stderr)
        return 1

    os.utime(a.output, None)
    return 0


if __name__ == "__main__":
    sys.exit(main())
