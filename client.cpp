#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void usage(int argc, char **argv) {
    printf("usage: %s <ip:port> <chunk1_id>,<chunk2_id>,...,<chunkN_id>\n", argv[0]);
    printf("example: %s 127. 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        usage(argc, argv);
    }

    char* ip_and_port = argv[1];
    char* peer_ip = strtok(ip_and_port, ":");
    char* peer_port = strtok(NULL, ":");

    // Criar socket UDP

    // Incializar dados para conexão com peer de contato

    // Criar lista de chunks requisitados

    // Criar N threads, de acordo com a quantidade de chunks requisitados

    // Essas threads irão esperar, por um tempo determinado, um recvfrom dos peers que contém os chunks
    // caso ultrapasse esse tempo de espera, será dado um timeout

    // Montar uma HELLO_MESSAGE

    // Enviar ao peer de contato

    // Dentro de cada thread:
    // - Criar uma mensagem GET
    // - Enviar a mensagem ao Peer que possui os chunks disponíveis
    // - Aguardar o recebimento da response
    // - Ao receber a response?
    //      - Abrir um arquivo pra escrita
    //      - Escrever o conteúdo do campo chunk da response no arquivo
    //      - salvar arquivo com o respectivo

    return(0);
}