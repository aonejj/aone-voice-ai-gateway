cmake_minimum_required(VERSION 3.15)
project(grpc_agent_client CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)       
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPCPP REQUIRED grpc++)
pkg_check_modules(PROTOBUF REQUIRED protobuf)
find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../proto/voice_agent.proto)
get_filename_component(PROTO_DIR ${PROTO_FILE} DIRECTORY)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

add_custom_command(
  OUTPUT ${GEN_DIR}/voice_agent.pb.cc ${GEN_DIR}/voice_agent.grpc.pb.cc
         ${GEN_DIR}/voice_agent.pb.h  ${GEN_DIR}/voice_agent.grpc.pb.h
  COMMAND ${PROTOC} --proto_path=${PROTO_DIR} --cpp_out=${GEN_DIR}
          --grpc_out=${GEN_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${PROTO_FILE}
  DEPENDS ${PROTO_FILE})

add_library(grpc_agent_client SHARED
  ${GEN_DIR}/voice_agent.pb.cc ${GEN_DIR}/voice_agent.grpc.pb.cc
  GRPCAgentChannel.cc GRPCAgentChannelManager.cc
  GRPCAgentStream.cc GRPCCompleteQueue.cc GRPCAgentSessionAPI.cc )

target_compile_definitions(grpc_agent_client PRIVATE _GLIBCXX_USE_CXX11_ABI=1)
target_include_directories(grpc_agent_client
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${GEN_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..
          ${GRPCPP_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIRS})
target_link_directories(grpc_agent_client PRIVATE ${GRPCPP_LIBRARY_DIRS} ${PROTOBUF_LIBRARY_DIRS})
target_link_libraries(grpc_agent_client PRIVATE
  ${GRPCPP_LIBRARIES} ${PROTOBUF_LIBRARIES} Threads::Threads)