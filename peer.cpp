#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <map>

#include "util.h"

#define MAX_BUFFER_SIZE 64

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

    // informacao da conexao recebida
    struct sockaddr_storage communicator_info;
    socklen_t communicator_addr_len;

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

    std::map<uint8_t, char*> key_values_file_map;

    if(!key_values_file.is_open())
        logexit("opening key values file error\n");

    while(std::getline(key_values_file, actual_key_values_file_line)) {
        trim(actual_key_values_file_line);
        char *key_values_file_string_to_tokenize = strdup(actual_key_values_file_line.c_str());
    
        char *key_values_file_id = strtok(key_values_file_string_to_tokenize, ":");
        char *key_values_file_name = strtok(NULL, ":");

        uint8_t key_values_file_id_int = parse_uint8(key_values_file_id);
        key_values_file_map[key_values_file_id_int] = key_values_file_name;
    }


    while(1) {
        // declarar um buffer vetor de char com um tamanho maior que suficiente para o recebimento
        char buffer_received[MAX_BUFFER_SIZE];
        
        // esperar a chegada de uma mensagem (recvfrom)
        int bytes_received = recvfrom(peer_sock, buffer_received, MAX_BUFFER_SIZE-1, 0, (struct sockaddr*)&communicator_info, &communicator_addr_len);

        if (bytes_received < 0) {
            logexit("recvfrom() error\n");
        }

        char msg_type_received[2];
        msg_type_received[0] = buffer_received[0];
        msg_type_received[1] = buffer_received[1];

        // identificar o tipo de mensagem (primeiros 2 bytes)
        uint16_t message_type_received = parse_uint16(msg_type_received);

        HELLO_MESSAGE hello_msg_received;
        GET_MESSAGE get_msg_received;
        QUERY_MESSAGE query_message_received;

        switch (message_type_received) {
            // caso seja QUERY:
            // Realizar um Type cast com o IP e Porto pra um uint e ushort
            // Type cast para ushort para a qtd de chunks
            // Alocar uma lista/vetor de ushort com o tamanho da quantidade
            case MESSAGE_TYPE::QUERY: 
                char client_ip_received[4];
                client_ip_received[0] = buffer_received[2];
                client_ip_received[1] = buffer_received[3];
                client_ip_received[2] = buffer_received[4];
                client_ip_received[3] = buffer_received[5];

                // char* client_ip_parsed = parse_ip_recv(client_ip_received);

                char client_port_received[2];
                client_port_received[0] = buffer_received[6];
                client_port_received[1] = buffer_received[7];
                
                // uint16_t port = parse_port_recv(client_port_received);

                char peer_ttl[2];
                peer_ttl[0] = buffer_received[8];
                peer_ttl[1] = buffer_received[9];

                char chunks_amount[2];
                chunks_amount[0] = buffer_received[10];
                chunks_amount[1] = buffer_received[11];

                // iterar sobre o buffer received para salvar a lista de chunks id
                break;
            
            case MESSAGE_TYPE::HELLO:
            case MESSAGE_TYPE::GET:
                // caso seja HELLO/GET:
                // Realizar um type cast pra ushort para a qtd de chunks
                // Alocar uma lista/vetor de ushort com o tamanho da quantidade

                char chunks_amount[2];
                chunks_amount[0] = buffer_received[10];
                chunks_amount[1] = buffer_received[11];

                // iterar sobre o buffer received para salvar a lista de chunks id

                break;
        }

        
        // Apos a inicialização das structs das mensagens
        
        // Caso seja msg HELLO:
        // Salvar informaçoes IP e Porto do cliente
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

        if (message_type_received == MESSAGE_TYPE::HELLO) {
            
        }

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