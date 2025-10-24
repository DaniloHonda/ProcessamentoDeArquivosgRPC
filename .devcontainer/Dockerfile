# Imagem base para desenvolvimento em C++ com ferramentas modernas
FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-22.04

# Instalar dependências adicionais
USER root

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    protobuf-compiler \
    libprotobuf-dev \
    libgrpc++-dev \
    grpc-proto \
    python3 \
    python3-pip \
    git \
    # Dependências adicionadas para o processamento de arquivos
    ghostscript \
    poppler-utils \
    imagemagick \
    && apt-get clean && rm -rf /var/lib/apt/lists/*