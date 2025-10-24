cmake_minimum_required(VERSION 3.10)
project(grpc_file_processor_server)

set(CMAKE_CXX_STANDARD 17)

# Encontra o utilitário pkg-config
find_package(PkgConfig REQUIRED)

# Usa o pkg-config para encontrar as bibliotecas gRPC e Protobuf
pkg_check_modules(gRPC REQUIRED grpc++)
pkg_check_modules(Protobuf REQUIRED protobuf)

# Encontra o compilador protoc
find_package(Protobuf REQUIRED)

# CORREÇÃO: Encontra o caminho para o plugin do gRPC usando um comando CMake
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

set(PROTO_FILE ../proto/file_processor.proto)
set(PROTO_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(
  OUTPUT ${PROTO_GENERATED_DIR}/file_processor.pb.cc ${PROTO_GENERATED_DIR}/file_processor.pb.h ${PROTO_GENERATED_DIR}/file_processor.grpc.pb.cc ${PROTO_GENERATED_DIR}/file_processor.grpc.pb.h
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
  --grpc_out=${PROTO_GENERATED_DIR}
  --cpp_out=${PROTO_GENERATED_DIR}
  -I../proto
  # CORREÇÃO: Usa a variável CMake em vez de um comando de shell
  --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
  ${PROTO_FILE}
  DEPENDS ${PROTO_FILE}
)

add_library(proto_lib
  ${PROTO_GENERATED_DIR}/file_processor.pb.cc
  ${PROTO_GENERATED_DIR}/file_processor.grpc.pb.cc
)
target_include_directories(proto_lib PUBLIC 
    ${PROTO_GENERATED_DIR}
    ${gRPC_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(proto_lib ${gRPC_LIBRARIES} ${Protobuf_LIBRARIES})

add_executable(server server.cpp)
target_link_libraries(server proto_lib)
target_include_directories(server PUBLIC ${PROTO_GENERATED_DIR})