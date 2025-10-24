import grpc
import os
import sys

# Adiciona o diretório proto ao path para encontrar os módulos gerados
sys.path.append(os.path.join(os.path.dirname(__file__), '../proto'))
import file_processor_pb2
import file_processor_pb2_grpc

CHUNK_SIZE = 1024 * 1024  # 1MB

def get_file_chunks(file_path):
    """Lê um arquivo e o retorna em pedaços (chunks)."""
    try:
        with open(file_path, 'rb') as f:
            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break
                yield file_processor_pb2.FileChunk(content=chunk)
    except FileNotFoundError:
        print(f"Erro: Arquivo '{file_path}' não encontrado.")
        return

def stream_compress_pdf(stub, input_path, output_path):
    """Chama o serviço de compressão de PDF."""
    print("Enviando arquivo para compressão...")
    response_iterator = stub.CompressPDF(get_file_chunks(input_path))
    with open(output_path, 'wb') as f:
        for chunk in response_iterator:
            f.write(chunk.content)
    print(f"PDF comprimido salvo em: {output_path}")

def stream_convert_to_txt(stub, input_path, output_path):
    """Chama o serviço de conversão para TXT."""
    print("Enviando arquivo para conversão TXT...")
    response_iterator = stub.ConvertToTXT(get_file_chunks(input_path))
    with open(output_path, 'wb') as f:
        for chunk in response_iterator:
            f.write(chunk.content)
    print(f"Arquivo de texto salvo em: {output_path}")

def stream_convert_image(stub, input_path, output_path, new_format):
    """Chama o serviço de conversão de formato de imagem."""
    def request_iterator():
        # Primeira mensagem com o parâmetro
        yield file_processor_pb2.ConvertImageRequest(output_format=new_format)
        # Chunks subsequentes com o conteúdo do arquivo
        with open(input_path, 'rb') as f:
            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break
                yield file_processor_pb2.ConvertImageRequest(content=chunk)

    print(f"Enviando imagem para converter para o formato '{new_format}'...")
    response_iterator = stub.ConvertImageFormat(request_iterator())
    with open(output_path, 'wb') as f:
        for chunk in response_iterator:
            f.write(chunk.content)
    print(f"Imagem convertida salva em: {output_path}")
    
def stream_resize_image(stub, input_path, output_path, width, height):
    """Chama o serviço de redimensionamento de imagem."""
    def request_iterator():
        # Primeira mensagem com os parâmetros
        dimensions = file_processor_pb2.Dimensions(width=width, height=height)
        yield file_processor_pb2.ResizeImageRequest(dimensions=dimensions)
        # Chunks subsequentes com o conteúdo do arquivo
        with open(input_path, 'rb') as f:
            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break
                yield file_processor_pb2.ResizeImageRequest(content=chunk)

    print(f"Enviando imagem para redimensionar para {width}x{height}...")
    response_iterator = stub.ResizeImage(request_iterator())
    with open(output_path, 'wb') as f:
        for chunk in response_iterator:
            f.write(chunk.content)
    print(f"Imagem redimensionada salva em: {output_path}")

def run():
    # Compilar o .proto para Python
    os.system('python3 -m grpc_tools.protoc -I../proto --python_out=../proto --grpc_python_out=../proto ../proto/file_processor.proto')

    with grpc.insecure_channel('localhost:50051') as channel:
        stub = file_processor_pb2_grpc.FileProcessorServiceStub(channel)
        
        while True:
            print("\n--- Cliente gRPC de Processamento de Arquivos ---")
            print("1. Comprimir PDF")
            print("2. Converter PDF para TXT")
            print("3. Converter formato de Imagem")
            print("4. Redimensionar Imagem")
            print("5. Sair")
            choice = input("Escolha uma opção: ")

            if choice == '1':
                in_path = input("Caminho do PDF de entrada: ")
                out_path = input("Caminho do PDF de saída: ")
                stream_compress_pdf(stub, in_path, out_path)
            elif choice == '2':
                in_path = input("Caminho do PDF de entrada: ")
                out_path = input("Caminho do arquivo TXT de saída: ")
                stream_convert_to_txt(stub, in_path, out_path)
            elif choice == '3':
                in_path = input("Caminho da imagem de entrada: ")
                out_path = input("Caminho da imagem de saída (ex: out.png): ")
                new_format = out_path.split('.')[-1]
                stream_convert_image(stub, in_path, out_path, new_format)
            elif choice == '4':
                in_path = input("Caminho da imagem de entrada: ")
                out_path = input("Caminho da imagem de saída: ")
                width = int(input("Largura desejada: "))
                height = int(input("Altura desejada: "))
                stream_resize_image(stub, in_path, out_path, width, height)
            elif choice == '5':
                break
            else:
                print("Opção inválida.")

if __name__ == '__main__':
    run()