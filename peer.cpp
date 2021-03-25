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
#include <sstream>
#include <map>

#include "util.h"

#define MAX_BUFFER_SIZE 32


void usage(int argc, char **argv) {
    printf("usage: %s <server port> <key-values-files_peer[id]> <ip1:port1> ... <ipN:portN>\n", argv[0]);
    printf("example: %s 127.0.0.1:51511 key-values-files_peer1 127.0.0.3:51513 127.0.0.4:51514\n", argv[0]);
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
    struct sockaddr_in communicator_info;
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

    std::vector<sockaddr_in> peer_neighboorhood;

    for(int i = 3; i < argc; i++ ) {
        char* ip_port_neigh_peer_i = argv[i];
        char* ip_neigh_peer_i = strtok(ip_port_neigh_peer_i, ":");
        char* port_neigh_peer_i = strtok(NULL, ":");

        struct sockaddr_in neigh_peer_info;
        addrparse(ip_neigh_peer_i, port_neigh_peer_i, &neigh_peer_info);
        peer_neighboorhood.push_back(neigh_peer_info);
    }

    // Ler arquivo key-value
    // Criar a lista que associa ID com string com nome dos chunks

    std::string actual_key_values_file_line;
    char* key_values_file_name;
    sprintf(key_values_file_name, "./Key-values-files/%s", argv[2]);
    std::ifstream key_values_file(key_values_file_name);

    std::map<uint16_t, char*> key_values_file_map;

    if(!key_values_file.is_open())
        logexit("opening key values file error\n");

    while(std::getline(key_values_file, actual_key_values_file_line)) {
        trim(actual_key_values_file_line);
        char *key_values_file_string_to_tokenize = strdup(actual_key_values_file_line.c_str());
    
        char *key_values_file_id = strtok(key_values_file_string_to_tokenize, ":");
        char *key_values_file_name = strtok(NULL, ":");

        uint16_t key_values_file_id_int = parse_uint16(key_values_file_id);
        key_values_file_map[key_values_file_id_int] = key_values_file_name;
    }

    QUERY_MESSAGE last_query_message_received;
    last_query_message_received.client_ip = 0;
    last_query_message_received.client_port = 0;

    while(1) {
        // declarar um buffer vetor de char com um tamanho maior que suficiente para o recebimento
        int16_t buffer_received[MAX_BUFFER_SIZE];
        
        // esperar a chegada de uma mensagem (recvfrom)
        int bytes_received = recvfrom(peer_sock, (void*)buffer_received, MAX_BUFFER_SIZE-1, 0, (struct sockaddr*)&communicator_info, &communicator_addr_len);

        if (bytes_received < 0) {
            logexit("recvfrom() error\n");
        }

        HELLO_MESSAGE hello_msg_received;
        GET_MESSAGE get_msg_received;
        QUERY_MESSAGE query_message_received;

        QUERY_MESSAGE query_msg_to_send;

        std::vector<uint16_t> client_chunks_id;

        // Montagem da mensagem recebida
        switch(ntohs(buffer_received[0])) {
            case (uint16_t)HELLO:
                hello_msg_received.chunks_amount = ntohs(buffer_received[1]);
                for(int16_t i = 0; i < hello_msg_received.chunks_amount; i++)
                    hello_msg_received.chunks_id[i] = ntohs(buffer_received[2+i]);
                break;
            case (uint16_t)GET:
                get_msg_received.chunks_amount = ntohs(buffer_received[1]);
                for(int16_t i = 0; i < get_msg_received.chunks_amount; i++)
                    get_msg_received.chunks_id[i] = ntohs(buffer_received[2+i]);
                break;
            case (uint16_t)QUERY:
                uint32_t *client_ip_received = (uint32_t*)&buffer_received[1];
                query_message_received.client_ip = ntohl(*client_ip_received);
                query_message_received.client_port = ntohs(buffer_received[3]);
                query_message_received.peer_ttl = ntohs(buffer_received[4]);
                query_message_received.chunks_amount = ntohs(buffer_received[5]);
                for(int16_t i = 0; i < query_message_received.chunks_amount; i++)
                    query_message_received.chunks_id[i] = ntohs(buffer_received[6+i]);
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
        // Ler o arquivo m4s correspondente, gravar e um vetor de char de 1024 bytes de tamanho
        // Montar uma mensagem RESPONSE:
        // chunk id
        // chunk size
        // chunk lido

        switch(ntohs(buffer_received[0])) {
            case (uint16_t)HELLO:
                CHUNKS_INFO_MESSAGE chunks_info_msg_to_send;

                int counter = 0;
                for (uint16_t &c_chunk_id : hello_msg_received.chunks_id) {
                    if(key_values_file_map.count(c_chunk_id) == 1) {
                        chunks_info_msg_to_send.chunks_amount++;
                        chunks_info_msg_to_send.chunks_id[counter] = htons(c_chunk_id);
                    } else {
                        query_msg_to_send.chunks_amount++;
                        query_msg_to_send.chunks_id[counter] = htons(c_chunk_id);
                    }
                    counter++;
                }

                if (chunks_info_msg_to_send.chunks_amount > 0) {
                    chunks_info_msg_to_send.msg_type = htons(chunks_info_msg_to_send.msg_type);
                    chunks_info_msg_to_send.chunks_amount = htons(chunks_info_msg_to_send.chunks_amount);

                    int num_bytes_sent = sendto(peer_sock, (void*)&chunks_info_msg_to_send, sizeof(CHUNKS_INFO_MESSAGE), 0, (sockaddr*)&communicator_info, communicator_addr_len);

                    if (num_bytes_sent < 0) {
                        logexit("chunks_info sendto() error\n");
                    }
                }

                if (query_msg_to_send.chunks_amount > 0) {
                    query_msg_to_send.msg_type = htons(query_msg_to_send.msg_type);
                    query_msg_to_send.client_ip = htonl((uint32_t)communicator_info.sin_addr.s_addr);
                    query_msg_to_send.client_port = htons((uint16_t)communicator_info.sin_port);
                    query_msg_to_send.peer_ttl = htons((uint16_t)3);
                    socklen_t neigh_peer_addr_len = (socklen_t)sizeof(sockaddr);
                    for (sockaddr_in &neigh_peer : peer_neighboorhood) {
                        int num_bytes_sent = sendto(peer_sock, (void*)&query_msg_to_send, sizeof(QUERY_MESSAGE), 0, (sockaddr*)&neigh_peer, neigh_peer_addr_len);

                        if (num_bytes_sent < 0) {
                            logexit("query sendto() error\n");
                        }
                    }
                }
                break;
            case (uint16_t)QUERY:
                bool query_already_received = last_query_message_received.client_ip == query_message_received.client_ip && last_query_message_received.client_port == query_message_received.client_port;

                if (query_already_received) {
                    continue;
                }

                struct sockaddr_in client_addr;
                socklen_t client_addr_len = (socklen_t)sizeof(sockaddr);
                client_addr.sin_addr.s_addr = query_message_received.client_ip;
                client_addr.sin_port = query_message_received.client_port;

                CHUNKS_INFO_MESSAGE chunks_info_msg_to_send;

                int counter = 0;
                for (uint16_t &c_chunk_id : query_message_received.chunks_id) {
                    if(key_values_file_map.count(c_chunk_id) == 1) {
                        chunks_info_msg_to_send.chunks_amount++;
                        chunks_info_msg_to_send.chunks_id[counter] = htons(c_chunk_id);
                    } else {
                        query_msg_to_send.chunks_amount++;
                        query_msg_to_send.chunks_id[counter] = htons(c_chunk_id);
                    }
                    counter++;
                }

                if (chunks_info_msg_to_send.chunks_amount > 0) {
                    chunks_info_msg_to_send.msg_type = htons(chunks_info_msg_to_send.msg_type);
                    chunks_info_msg_to_send.chunks_amount = htons(chunks_info_msg_to_send.chunks_amount);

                    int num_bytes_sent = sendto(peer_sock, (void*)&chunks_info_msg_to_send, sizeof(CHUNKS_INFO_MESSAGE), 0, (sockaddr*)&client_addr, client_addr_len);

                    if (num_bytes_sent < 0) {
                        logexit("chunks_info sendto() error\n");
                    }
                }

                if (query_msg_to_send.chunks_amount > 0 && query_msg_to_send.peer_ttl > 0) {
                    query_msg_to_send.msg_type = htons(query_msg_to_send.msg_type);
                    query_msg_to_send.client_ip = htonl(query_message_received.client_ip);
                    query_msg_to_send.client_port = htons(query_message_received.client_port);
                    query_msg_to_send.peer_ttl = htons(query_message_received.peer_ttl - 1);
                    query_msg_to_send.chunks_amount = htons(query_msg_to_send.chunks_amount);

                    socklen_t neigh_peer_addr_len = (socklen_t)sizeof(sockaddr);
                    for (sockaddr_in &neigh_peer : peer_neighboorhood) {
                        int num_bytes_sent = sendto(peer_sock, (void*)&query_msg_to_send, sizeof(QUERY_MESSAGE), 0, (sockaddr*)&neigh_peer, neigh_peer_addr_len);

                        if (num_bytes_sent < 0) {
                            logexit("query sendto() error\n");
                        }
                    }
                }
                break;
            case (uint16_t)GET:
                for (int16_t chunk_id : get_msg_received.chunks_id) {
                    char* chunk_file_name;
                    sprintf(chunk_file_name, "./Chunks/BigBuckBunny_%d.m4s", chunk_id);
                    char* chunk_file_content = (char*)calloc(1024, sizeof(char));
                    std::ifstream chunk_file(chunk_file_name);
                    
                    if(!chunk_file.is_open()) {
                        logexit("chunk file open() error\n");
                    }

                    std::stringstream buffer;
                    buffer << chunk_file.rdbuf();
                    
                    chunk_file_content = strdup(buffer.str().c_str());

                    RESPONSE_MESSAGE response_message_to_send;
                    response_message_to_send.msg_type = htons(response_message_to_send.msg_type);
                    response_message_to_send.chunk_id = htons(chunk_id);
                    response_message_to_send.chunk_size = htons((uint16_t)1024);
                    strncpy(response_message_to_send.chunk, chunk_file_content, 1024);

                    struct sockaddr_in client_addr;
                    socklen_t client_addr_len = (socklen_t)sizeof(sockaddr);
                    client_addr.sin_addr.s_addr = query_message_received.client_ip;
                    client_addr.sin_port = query_message_received.client_port;

                    int num_bytes_sent = sendto(peer_sock, (void*)&response_message_to_send, sizeof(RESPONSE_MESSAGE), 0, (sockaddr*)&client_addr, client_addr_len);

                    if (num_bytes_sent < 0) {
                        logexit("Response sendto() error\n");
                    }
                }
                break;
        }
    }

    return(0);
}