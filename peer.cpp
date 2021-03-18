#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <map>

#include "util.h"

struct neighboor_peer
{
    char* peer_ip;
    char* peer_port;
};

void usage(int argc, char **argv) {
    printf("usage: %s <server port> <key-values-files_peer[id]> <ip1:port1> ... <ipN:portN>\n", argv[0]);
    printf("example: %s 127. 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage(argc, argv);
    }
    
    char* ip_and_port = argv[1];
    char* peer_ip = strtok(ip_and_port, ":");
    char* peer_port = strtok(NULL, ":");

    struct addrinfo peerInfo, *res;
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.ai_family = AF_INET;
    peerInfo.ai_flags = AI_NUMERICHOST;
    peerInfo.ai_socktype = SOCK_DGRAM;
    peerInfo.ai_protocol = IPPROTO_UDP;

    int rtnVal = getaddrinfo(peer_ip, peer_port, &peerInfo, &res);
    if (rtnVal != 0)
        logexit("getaddrinfo() error\n");

    int peer_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (peer_sock < 0)
        logexit("socket() error\n");

    if (bind(peer_sock, res->ai_addr, res->ai_addrlen) < 0)
        logexit("bind() error\n");

    freeaddrinfo(res);

    std::vector<struct neighboor_peer> peer_neighboorhood;

    for(int i = 3; i < argc; i++ ) {
        char* ip_port_neigh_peer_i = argv[i];
        char* ip_neigh_peer_i = strtok(ip_port_neigh_peer_i, ":");
        char* port_neigh_peer_i = strtok(NULL, ":");

        struct neighboor_peer neigh_peer_i = {.peer_ip = ip_neigh_peer_i, .peer_port = port_neigh_peer_i};

        peer_neighboorhood.push_back(neigh_peer_i);
    }

    // Ler arquivo key-value
    // Criar a lista que associa ID com string com nome dos chunks

    std::string actual_key_values_file_line;
    std::ifstream key_values_file(argv[2]);

    std::map<int, char*> key_values_file_map;

    if(!key_values_file.is_open())
        logexit("opening key values file error\n");

    while(std::getline(key_values_file, actual_key_values_file_line)) {
        trim(actual_key_values_file_line);
        char *key_values_file_string_to_tokenize = strdup(actual_key_values_file_line.c_str());
    
        char *key_values_file_id = strtok(key_values_file_string_to_tokenize, ":");
        char *key_values_file_name = strtok(NULL, ":");

        int key_values_file_id_int = parseInt(key_values_file_id);
        key_values_file_map[key_values_file_id_int] = key_values_file_name;
    }


    while(1) {
        // declarar um buffer vetor de char com um tamanho maior que suficiente para o recebimento
        
        // esperar a chegada de uma mensagem (recvfrom)

        // identificar o tipo de mensagem (primeiros 2 bytes)

        // caso seja QUERY:
        // Realizar um Type cast com o IP e Porto pra um uint e ushort
        // Type cast para ushort para a qtd de chunks
        // Alocar uma lista/vetor de ushort com o tamanho da quantidade

        // caso seja HELLO/GET:
        // Realizar um type cast pra ushort para a qtd de chunks
        // Alocar uma lista/vetor de ushort com o tamanho da quantidade

        // Apos a inicialização das structs das mensagens
        
        // Caso seja msg HELLO:
        // Verificar se possui algum chunk rquisitado pelo cliente
        // Montar a mensagem CHUNKS_INFO
        // enviar pro cliente via sendto
        // 
        // Caso ainda existam chunks a serem enviados:
        // Montar mensagem QUERY
        // IP e Porto do cliente que enviou
        // Peer-TTL = 3
        // Quantidade de chunks: Todos recebidos - Contido pelo peer atual
        // Enviar mensagem QUERY para todos os peers vizinhos

        // Caso seja msg QUERY:
        // Verificar lista de chunks recebida e se há um ou mais chunks contidos
        // Caso sim Montar a mensagem CHUNKS_INFO
        // enviar pro cliente via sendto
        //
        // Caso existam chunks a serem enviados E Peer-TTL > 0
        // Montar mensagem QUERY
        // IP e Porto do cliente que enviou
        // Peer-TTL = ttl_atual - 1
        // Quantidade de chunks: Todos recebidos menos os contidos pelo peer atual
        // Enviar mensagem QUERY para todos os peers vizinhos

        // Caso seja msg GET:
        // 
        // Verificar a lista de chunks solicitados
        // Para cada chunk solicitado:
        // Ler o arquivo m4s correspondente, gravar e um vetor de char de 1000 bytes de tamanho
        // Montar uma mensagem RESPONSE:
        // chunk id
        // chunk size
        // chunk lido
    }

    return(0);
}