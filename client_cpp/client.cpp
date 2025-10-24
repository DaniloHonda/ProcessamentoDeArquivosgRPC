#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <limits> // Para numeric_limits

#include <grpcpp/grpcpp.h>
#include "file_processor.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace file_processor;

const int CHUNK_SIZE = 1024 * 1024; // 1MB

class FileProcessorClient {
public:
    FileProcessorClient(std::shared_ptr<Channel> channel)
        : stub_(FileProcessorService::NewStub(channel)) {}

    void CompressPDF(const std::string& input_path, const std::string& output_path) {
        ClientContext context;
        auto stream = stub_->CompressPDF(&context);

        std::ifstream input_file(input_path, std::ios::binary);
        if (!input_file.is_open()) {
            std::cerr << "Erro: Não foi possível abrir o arquivo de entrada '" << input_path << "'." << std::endl;
            stream->WritesDone();
            return;
        }
        std::cout << "Enviando arquivo para compressão..." << std::endl;
        
        char buffer[CHUNK_SIZE];
        while (input_file.read(buffer, sizeof(buffer))) {
            FileChunk chunk;
            chunk.set_content(buffer, input_file.gcount());
            if (!stream->Write(chunk)) break;
        }
        if (input_file.gcount() > 0) {
            FileChunk chunk;
            chunk.set_content(buffer, input_file.gcount());
            stream->Write(chunk);
        }
        stream->WritesDone();

        std::ofstream output_file(output_path, std::ios::binary);
        FileChunk received_chunk;
        while (stream->Read(&received_chunk)) {
            output_file.write(received_chunk.content().c_str(), received_chunk.content().length());
        }

        Status status = stream->Finish();
        if (status.ok()) {
            std::cout << "PDF comprimido e salvo em: " << output_path << std::endl;
        } else {
            std::cerr << "RPC falhou: " << status.error_message() << std::endl;
        }
    }
    
    void ConvertToTXT(const std::string& input_path, const std::string& output_path) {
        ClientContext context;
        auto stream = stub_->ConvertToTXT(&context);

        std::ifstream input_file(input_path, std::ios::binary);
        if (!input_file.is_open()) {
            std::cerr << "Erro: Não foi possível abrir o arquivo de entrada '" << input_path << "'." << std::endl;
            stream->WritesDone();
            return;
        }
        std::cout << "Enviando arquivo para conversão TXT..." << std::endl;

        char buffer[CHUNK_SIZE];
        while (input_file.read(buffer, sizeof(buffer))) {
            FileChunk chunk;
            chunk.set_content(buffer, input_file.gcount());
            if (!stream->Write(chunk)) break;
        }
        if (input_file.gcount() > 0) {
            FileChunk chunk;
            chunk.set_content(buffer, input_file.gcount());
            stream->Write(chunk);
        }
        stream->WritesDone();

        std::ofstream output_file(output_path, std::ios::binary);
        FileChunk received_chunk;
        while (stream->Read(&received_chunk)) {
            output_file.write(received_chunk.content().c_str(), received_chunk.content().length());
        }

        Status status = stream->Finish();
        if (status.ok()) {
            std::cout << "Arquivo de texto salvo em: " << output_path << std::endl;
        } else {
            std::cerr << "RPC falhou: " << status.error_message() << std::endl;
        }
    }

    void ConvertImageFormat(const std::string& input_path, const std::string& output_path, const std::string& format) {
        ClientContext context;
        auto stream = stub_->ConvertImageFormat(&context);
        
        std::ifstream input_file(input_path, std::ios::binary);
        if (!input_file.is_open()) {
            std::cerr << "Erro: Não foi possível abrir o arquivo de entrada '" << input_path << "'." << std::endl;
            stream->WritesDone();
            return;
        }
        std::cout << "Enviando imagem para conversão para o formato " << format << "..." << std::endl;

        // Primeira mensagem com o parâmetro
        ConvertImageRequest initial_request;
        initial_request.set_output_format(format);
        stream->Write(initial_request);

        // Chunks subsequentes com o conteúdo do arquivo
        char buffer[CHUNK_SIZE];
        while (input_file.read(buffer, sizeof(buffer))) {
            ConvertImageRequest request;
            request.set_content(buffer, input_file.gcount());
            if (!stream->Write(request)) break;
        }
        if (input_file.gcount() > 0) {
            ConvertImageRequest request;
            request.set_content(buffer, input_file.gcount());
            stream->Write(request);
        }
        stream->WritesDone();

        std::ofstream output_file(output_path, std::ios::binary);
        FileChunk received_chunk;
        while (stream->Read(&received_chunk)) {
            output_file.write(received_chunk.content().c_str(), received_chunk.content().length());
        }

        Status status = stream->Finish();
        if (status.ok()) {
            std::cout << "Imagem convertida salva em: " << output_path << std::endl;
        } else {
            std::cerr << "RPC falhou: " << status.error_message() << std::endl;
        }
    }
    
    void ResizeImage(const std::string& input_path, const std::string& output_path, int width, int height) {
        ClientContext context;
        auto stream = stub_->ResizeImage(&context);
        
        std::ifstream input_file(input_path, std::ios::binary);
        if (!input_file.is_open()) {
            std::cerr << "Erro: Não foi possível abrir o arquivo de entrada '" << input_path << "'." << std::endl;
            stream->WritesDone();
            return;
        }
        std::cout << "Enviando imagem para redimensionar para " << width << "x" << height << "..." << std::endl;

        // Primeira mensagem com os parâmetros
        ResizeImageRequest initial_request;
        initial_request.mutable_dimensions()->set_width(width);
        initial_request.mutable_dimensions()->set_height(height);
        stream->Write(initial_request);
        
        // Chunks subsequentes com o conteúdo do arquivo
        char buffer[CHUNK_SIZE];
        while (input_file.read(buffer, sizeof(buffer))) {
            ResizeImageRequest request;
            request.set_content(buffer, input_file.gcount());
            if (!stream->Write(request)) break;
        }
        if (input_file.gcount() > 0) {
            ResizeImageRequest request;
            request.set_content(buffer, input_file.gcount());
            stream->Write(request);
        }
        stream->WritesDone();

        // Receber arquivo
        std::ofstream output_file(output_path, std::ios::binary);
        FileChunk received_chunk;
        while (stream->Read(&received_chunk)) {
            output_file.write(received_chunk.content().c_str(), received_chunk.content().length());
        }

        Status status = stream->Finish();
        if (status.ok()) {
            std::cout << "Imagem redimensionada salva em: " << output_path << std::endl;
        } else {
            std::cerr << "RPC falhou: " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<FileProcessorService::Stub> stub_;
};

void print_menu() {
    std::cout << "\n--- Cliente gRPC de Processamento de Arquivos (C++) ---\n";
    std::cout << "1. Comprimir PDF\n";
    std::cout << "2. Converter PDF para TXT\n";
    std::cout << "3. Converter formato de Imagem\n";
    std::cout << "4. Redimensionar Imagem\n";
    std::cout << "5. Sair\n";
    std::cout << "Escolha uma opção: ";
}

int main() {
    std::string server_address("localhost:50051");
    FileProcessorClient client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    int choice = 0;
    while (choice != 5) {
        print_menu();
        std::cin >> choice;
        
        if (std::cin.fail()) {
            std::cin.clear(); // Limpa o estado de erro
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Descarta a entrada inválida
            choice = 0; // Reseta a escolha para evitar loop infinito
            std::cout << "Entrada inválida. Por favor, insira um número." << std::endl;
            continue;
        }
        
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Limpa o buffer de entrada para o próximo getline

        std::string in_path, out_path, format;
        int width, height;

        switch (choice) {
            case 1:
                std::cout << "Caminho do PDF de entrada: ";
                std::getline(std::cin, in_path);
                std::cout << "Caminho do PDF de saída: ";
                std::getline(std::cin, out_path);
                client.CompressPDF(in_path, out_path);
                break;
            case 2:
                std::cout << "Caminho do PDF de entrada: ";
                std::getline(std::cin, in_path);
                std::cout << "Caminho do arquivo TXT de saída: ";
                std::getline(std::cin, out_path);
                client.ConvertToTXT(in_path, out_path);
                break;
            case 3:
                std::cout << "Caminho da imagem de entrada: ";
                std::getline(std::cin, in_path);
                std::cout << "Caminho da imagem de saída (ex: out.png): ";
                std::getline(std::cin, out_path);
                std::cout << "Novo formato (ex: png): ";
                std::getline(std::cin, format);
                client.ConvertImageFormat(in_path, out_path, format);
                break;
            case 4:
                std::cout << "Caminho da imagem de entrada: ";
                std::getline(std::cin, in_path);
                std::cout << "Caminho da imagem de saída: ";
                std::getline(std::cin, out_path);
                std::cout << "Largura desejada: ";
                std::cin >> width;
                std::cout << "Altura desejada: ";
                std::cin >> height;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                client.ResizeImage(in_path, out_path, width, height);
                break;
            case 5:
                std::cout << "Saindo..." << std::endl;
                break;
            default:
                std::cout << "Opção inválida." << std::endl;
        }
    }

    return 0;
}