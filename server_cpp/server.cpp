#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <cstdio>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random> // Adicionado para nomes de arquivo únicos

#include <grpcpp/grpcpp.h>
#include "file_processor.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using namespace file_processor;

// Função para obter o timestamp atual formatado
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Função de logging
void logOperation(const std::string& service_name, const std::string& status, const std::string& message) {
    std::ofstream log_file("server.log", std::ios::app);
    std::string log_entry = "[" + getCurrentTimestamp() + "] [" + service_name + "] [" + status + "] " + message;
    if (log_file.is_open()) {
        log_file << log_entry << std::endl;
    }
    std::cout << log_entry << std::endl;
}

// Função para gerar um nome de arquivo aleatório e único
std::string generateUniqueFilename(const std::string& prefix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(100000, 999999);
    return "/tmp/" + prefix + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "_" + std::to_string(distrib(gen));
}


// Classe de implementação do serviço
class FileProcessorServiceImpl final : public FileProcessorService::Service {
private:
    // Função auxiliar para enviar o arquivo via stream
    bool sendFile(ServerReaderWriter<FileChunk, FileChunk>* stream, const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        char buffer[4096];
        while (file.read(buffer, sizeof(buffer))) {
            FileChunk chunk;
            chunk.set_content(buffer, file.gcount());
            if (!stream->Write(chunk)) break;
        }
        if (file.gcount() > 0) {
            FileChunk chunk;
            chunk.set_content(buffer, file.gcount());
            stream->Write(chunk);
        }
        return true;
    }

public:
    Status CompressPDF(ServerContext* context, ServerReaderWriter<FileChunk, FileChunk>* stream) override {
        logOperation("CompressPDF", "INFO", "Requisição recebida.");
        std::string input_path = generateUniqueFilename("input_compress");
        std::string output_path = input_path + "_out.pdf";

        std::ofstream temp_file(input_path, std::ios::binary);
        if (!temp_file.is_open()) {
            logOperation("CompressPDF", "ERROR", "Falha ao criar arquivo temporário de entrada.");
            return Status(grpc::StatusCode::INTERNAL, "Erro ao salvar arquivo de entrada.");
        }
        FileChunk chunk;
        while (stream->Read(&chunk)) {
            temp_file.write(chunk.content().c_str(), chunk.content().length());
        }
        temp_file.close();
        
        std::string command = "gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/ebook -dNOPAUSE -dQUIET -dBATCH -sOutputFile=" + output_path + " " + input_path;
        int result = std::system(command.c_str());

        if (result == 0) {
            if (!sendFile(stream, output_path)) {
                logOperation("CompressPDF", "ERROR", "Falha ao enviar arquivo comprimido.");
                return Status(grpc::StatusCode::INTERNAL, "Erro ao enviar arquivo de saída.");
            }
            logOperation("CompressPDF", "SUCCESS", "Arquivo comprimido e enviado com sucesso.");
        } else {
            logOperation("CompressPDF", "ERROR", "Falha na execução do Ghostscript. Código: " + std::to_string(result));
            return Status(grpc::StatusCode::INTERNAL, "Falha ao comprimir PDF.");
        }

        std::remove(input_path.c_str());
        std::remove(output_path.c_str());
        return Status::OK;
    }

    Status ConvertToTXT(ServerContext* context, ServerReaderWriter<FileChunk, FileChunk>* stream) override {
        logOperation("ConvertToTXT", "INFO", "Requisição recebida.");
        std::string input_path = generateUniqueFilename("input_totext");
        std::string output_path = input_path + "_out.txt";

        std::ofstream temp_file(input_path, std::ios::binary);
        if (!temp_file.is_open()) {
             logOperation("ConvertToTXT", "ERROR", "Falha ao salvar arquivo temporário.");
             return Status(grpc::StatusCode::INTERNAL, "Erro ao salvar arquivo de entrada.");
        }
        FileChunk chunk;
        while (stream->Read(&chunk)) {
            temp_file.write(chunk.content().c_str(), chunk.content().length());
        }
        temp_file.close();

        std::string command = "pdftotext " + input_path + " " + output_path;
        int result = std::system(command.c_str());

        if (result == 0) {
            if (!sendFile(stream, output_path)) {
                logOperation("ConvertToTXT", "ERROR", "Falha ao enviar arquivo de texto.");
                return Status(grpc::StatusCode::INTERNAL, "Erro ao enviar arquivo de saída.");
            }
            logOperation("ConvertToTXT", "SUCCESS", "Arquivo convertido e enviado com sucesso.");
        } else {
            logOperation("ConvertToTXT", "ERROR", "Falha na execução do pdftotext. Código: " + std::to_string(result));
            return Status(grpc::StatusCode::INTERNAL, "Falha ao converter PDF para TXT.");
        }

        std::remove(input_path.c_str());
        std::remove(output_path.c_str());
        return Status::OK;
    }

    Status ConvertImageFormat(ServerContext* context, ServerReaderWriter<ConvertImageRequest, FileChunk>* stream) override {
        logOperation("ConvertImageFormat", "INFO", "Requisição recebida.");
        
        ConvertImageRequest request;
        if (!stream->Read(&request) || !request.has_output_format()) {
            logOperation("ConvertImageFormat", "ERROR", "Primeira mensagem não continha o formato de saída.");
            return Status(grpc::StatusCode::INVALID_ARGUMENT, "A primeira mensagem deve conter o formato de saída.");
        }
        std::string format = request.output_format();

        std::string input_path = generateUniqueFilename("input_convert");
        std::string output_path = input_path + "_out." + format;
        
        std::ofstream temp_file(input_path, std::ios::binary);
        if(!temp_file.is_open()){
            logOperation("ConvertImageFormat", "ERROR", "Falha ao criar arquivo temporário.");
            return Status(grpc::StatusCode::INTERNAL, "Erro ao salvar arquivo de entrada.");
        }

        // Continua a ler o restante dos chunks de conteúdo
        while (stream->Read(&request)) {
            temp_file.write(request.content().c_str(), request.content().length());
        }
        temp_file.close();

        std::string command = "convert " + input_path + " " + output_path;
        int result = std::system(command.c_str());

        if (result == 0) {
            // Reusa a função sendFile para FileChunk
            if (!sendFile(reinterpret_cast<ServerReaderWriter<FileChunk, FileChunk>*>(stream), output_path)) {
                logOperation("ConvertImageFormat", "ERROR", "Falha ao enviar imagem convertida.");
                return Status(grpc::StatusCode::INTERNAL, "Erro ao enviar arquivo de saída.");
            }
            logOperation("ConvertImageFormat", "SUCCESS", "Imagem convertida e enviada com sucesso.");
        } else {
            logOperation("ConvertImageFormat", "ERROR", "Falha na execução do convert. Código: " + std::to_string(result));
            return Status(grpc::StatusCode::INTERNAL, "Falha ao converter imagem.");
        }
        
        std::remove(input_path.c_str());
        std::remove(output_path.c_str());
        return Status::OK;
    }

    Status ResizeImage(ServerContext* context, ServerReaderWriter<ResizeImageRequest, FileChunk>* stream) override {
        logOperation("ResizeImage", "INFO", "Requisição recebida.");
        
        ResizeImageRequest request;
        if (!stream->Read(&request) || !request.has_dimensions()) {
            logOperation("ResizeImage", "ERROR", "Primeira mensagem não continha as dimensões.");
            return Status(grpc::StatusCode::INVALID_ARGUMENT, "A primeira mensagem deve conter as dimensões.");
        }
        int width = request.dimensions().width();
        int height = request.dimensions().height();

        std::string input_path = generateUniqueFilename("input_resize");
        std::string output_path = input_path + "_out";

        std::ofstream temp_file(input_path, std::ios::binary);
        if(!temp_file.is_open()){
            logOperation("ResizeImage", "ERROR", "Falha ao criar arquivo temporário.");
            return Status(grpc::StatusCode::INTERNAL, "Erro ao salvar arquivo de entrada.");
        }

        while (stream->Read(&request)) {
            temp_file.write(request.content().c_str(), request.content().length());
        }
        temp_file.close();

        std::string command = "convert " + input_path + " -resize " + std::to_string(width) + "x" + std::to_string(height) + "! " + output_path;
        int result = std::system(command.c_str());

        if (result == 0) {
            if (!sendFile(reinterpret_cast<ServerReaderWriter<FileChunk, FileChunk>*>(stream), output_path)) {
                logOperation("ResizeImage", "ERROR", "Falha ao enviar imagem redimensionada.");
                return Status(grpc::StatusCode::INTERNAL, "Erro ao enviar arquivo de saída.");
            }
            logOperation("ResizeImage", "SUCCESS", "Imagem redimensionada e enviada com sucesso.");
        } else {
            logOperation("ResizeImage", "ERROR", "Falha na execução do convert. Código: " + std::to_string(result));
            return Status(grpc::StatusCode::INTERNAL, "Falha ao redimensionar imagem.");
        }
        
        std::remove(input_path.c_str());
        std::remove(output_path.c_str());
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    FileProcessorServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Servidor gRPC ouvindo em " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}