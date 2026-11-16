#!/bin/bash
set -e
cd "$(dirname "$0")"
cmake -S . -B ../out/grpc_agent_client -DCMAKE_BUILD_TYPE=${1:-RelWithDebInfo}
cmake --build ../out/grpc_agent_client -j