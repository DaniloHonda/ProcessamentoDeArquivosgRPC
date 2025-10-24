cmake_minimum_required(VERSION 3.10)
project(grpc_file_processor_client)

set(CMAKE_CXX_STANDARD 17)

# Usa o pkg-config para encontrar gRPC e Protobuf
find_package(PkgConfig REQUIRED)
pkg_check_modules(gRPC REQUIRED grpc++)
pkg_check_modules(Protobuf REQUIRED protobuf)

# Aponta para o diretório onde o servidor compilou os arquivos .proto
set(PROTO_GENERATED_DIR ../server_cpp/build)

# Adiciona o executável do cliente
add_executable(client client.cpp)

# Vincula o cliente às bibliotecas necessárias encontradas pelo pkg-config
target_link_libraries(client 
    ${gRPC_LIBRARIES} 
    ${Protobuf_LIBRARIES}
)

# Inclui os diretórios de cabeçalho do gRPC, Protobuf e dos arquivos gerados
target_include_directories(client PUBLIC 
    ${PROTO_GENERATED_DIR}
    ${gRPC_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
)