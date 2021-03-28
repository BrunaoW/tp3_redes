#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include "util.h"

#define MAX_BUFFER_SIZE 1128

#define PORT 51511

std::map<uint16_t, struct client_info_for_thread> info_used_in_threads;

void usage(int argc, char **argv) {
    printf("usage: %s <ip:port> <chunk1_id>,<chunk2_id>,...,<chunkN_id>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_info_for_thread {
    uint16_t msg_received;
    in_addr ip_received;
    uint16_t port_received = 0;
    uint16_t chunk_id_associated;
};

void* receive_chunk_info_thread(void* client_info) {
    int client_socket = *((int*)client_info);
    struct client_info_for_thread c_info_for_thread;
    uint16_t buffer_received[MAX_BUFFER_SIZE];
    struct sockaddr_in peer_connecting_info;
    socklen_t communicator_addr_len = sizeof(struct sockaddr_in);

    printf("Waiting message...\n");

    // Esperar recebimento de mensagem CHUNKS_INFO ou RESPONSE
    int bytes_received = recvfrom(client_socket, (void*)buffer_received, MAX_BUFFER_SIZE-1, 0, (struct sockaddr*)&peer_connecting_info, &communicator_addr_len);
    if (bytes_received < 0) {
        printf("Search thread timeout - The socket wasnt reachable or all the sockets were found in lesser threads\n");

        pthread_exit((void*)EXIT_FAILURE);
    }

    CHUNKS_INFO_MESSAGE chunks_info_received;
    RESPONSE_MESSAGE response_message_received;

    switch(ntohs(buffer_received[0])) {
        case CHUNKS_INFO:
        {
            printf("CHUNKS INFO message received - %s : %d\n", inet_ntoa(peer_connecting_info.sin_addr), ntohs(peer_connecting_info.sin_port));
            c_info_for_thread.msg_received = CHUNKS_INFO;
            
            // salvar ip e porto do peer que enviou
            c_info_for_thread.ip_received = peer_connecting_info.sin_addr;
            c_info_for_thread.port_received = peer_connecting_info.sin_port;

            chunks_info_received.chunks_amount = ntohs(buffer_received[1]);
            for (uint16_t i = 0; i < chunks_info_received.chunks_amount; i++)
                chunks_info_received.chunks_id[i] = ntohs(buffer_received[i + 2]);

            printf("Creating GET message to send\n");
            GET_MESSAGE get_message_to_send;
            get_message_to_send.msg_type = htons(get_message_to_send.msg_type);
            get_message_to_send.chunks_amount = htons(chunks_info_received.chunks_amount);
            for (uint16_t i = 0; i < chunks_info_received.chunks_amount; i++) {
                get_message_to_send.chunks_id[i] = ntohs(chunks_info_received.chunks_id[i]);
                c_info_for_thread.chunk_id_associated = chunks_info_received.chunks_id[i];
                info_used_in_threads[chunks_info_received.chunks_id[i]] = c_info_for_thread;
            }

            int bytes_sent = sendto(client_socket, (void*)&get_message_to_send, sizeof(GET_MESSAGE), 0, (struct sockaddr*)&peer_connecting_info, communicator_addr_len);
            if (bytes_sent < 0) {
                logexit("sendto() error\n");
            }
            printf("GET message sent\n");

            pthread_exit(EXIT_SUCCESS);
            break;
        }
        case RESPONSE:
        {
            printf("RESPONSE message received - %s : %d\n", inet_ntoa(peer_connecting_info.sin_addr), ntohs(peer_connecting_info.sin_port));
            c_info_for_thread.msg_received = RESPONSE;

            uint16_t chunk_received_id = ntohs(buffer_received[1]);
            c_info_for_thread.chunk_id_associated = chunk_received_id;
            response_message_received.chunk_id = chunk_received_id;
            response_message_received.chunk_size = ntohs(buffer_received[2]);
            
            // - Abrir um arquivo para escrita
            printf("Writing chunk in file...\n");
            char file_name[32];
            sprintf(file_name, "received_chunk_%d.m4s", chunk_received_id);

            char* chunk_content_char_array = (char*)&buffer_received[3];

            // - Escrever o buffer recebido no arquivo
            std::ofstream write_file(file_name);
            
            if(!write_file.is_open()) {
                logexit("open file error\n");
            }

            write_file.write(chunk_content_char_array, sizeof(char) * 1024);

            // - salvar arquivo com o respectivo nome do chunk
            write_file.close();
            
            // - salvar ip e porto do peer que enviou
            c_info_for_thread.ip_received = peer_connecting_info.sin_addr;
            c_info_for_thread.port_received = peer_connecting_info.sin_port;

            printf("Chunk with id %d retrieved successfully\n", chunk_received_id);

            // - encerrar thread
            pthread_exit(EXIT_SUCCESS);
            break;
        }
    }

    pthread_exit((void*)EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        usage(argc, argv);
    }

    char* ip_and_port = argv[1];
    char* peer_ip = strtok(ip_and_port, ":");
    char* peer_port = strtok(NULL, ":");

    struct addrinfo contact_peer_info;

    memset(&contact_peer_info, 0, sizeof(struct addrinfo));
    contact_peer_info.ai_family = AF_INET;
    contact_peer_info.ai_socktype = SOCK_DGRAM;
    contact_peer_info.ai_protocol = IPPROTO_UDP;
    contact_peer_info.ai_flags = AI_PASSIVE;

    struct addrinfo *peer_addr;
    int rtnVal = getaddrinfo(peer_ip, peer_port, &contact_peer_info, &peer_addr);
    if (rtnVal != 0) {
        logexit("getaddrinfo() error\n");
    }

    // Criar lista de chunks requisitados
    char* chunks_id = argv[2];
    char* chunk_id_str;
    std::vector<uint16_t> chunks_id_read;

    chunk_id_str = strtok(chunks_id, ",");

    while(chunk_id_str != NULL) {
        chunks_id_read.push_back(parse_uint16(chunk_id_str));
        chunk_id_str = strtok(NULL, ",");
    }

    int client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_sock < 0) {
        logexit("socket() error\n");
    }

    int enable = 1;
    if (0 != setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))){
        logexit("UDP setsockopt");
    }

    // atribui um timeout
    struct timeval tv;
    tv.tv_sec = 6;
    tv.tv_usec = 500000;
    if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv)) < 0) {
        perror("Error creating timeout");
    }

    // Criar N threads, de acordo com a quantidade de chunks requisitados
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(PORT);

    if (bind(client_sock, (struct sockaddr*)&local, sizeof(local)) < 0)
        logexit("bind() error");

    std::vector<pthread_t> chunk_search_threads;

    for (uint16_t &chunk_id : chunks_id_read) {
        printf("Setting up chunks receivement\n");
        pthread_t chunk_thread_id;
        pthread_t alternate_chunk_thread_id;

        struct client_info_for_thread c_info;
        c_info.chunk_id_associated = chunk_id;
        c_info.ip_received.s_addr = 0;
        c_info.port_received = 0; 
        info_used_in_threads[chunk_id] = c_info; 

        if (pthread_create(&chunk_thread_id, NULL, &receive_chunk_info_thread, (void*)&client_sock) != 0) {
            logexit("pthread_create() error\n");
        }

        if (pthread_create(&alternate_chunk_thread_id, NULL, &receive_chunk_info_thread, (void*)&client_sock) != 0) {
            logexit("pthread_create() error\n");
        }

        chunk_search_threads.push_back(chunk_thread_id);
        chunk_search_threads.push_back(alternate_chunk_thread_id);
    }

    // Essas threads irão esperar, por um tempo determinado, um recvfrom dos peers que contém os chunks
    // caso ultrapasse esse tempo de espera, será dado um timeout

    // Montar uma HELLO_MESSAGE
    printf("Creating HELLO message\n");

    HELLO_MESSAGE hello_message_to_send;
    hello_message_to_send.msg_type = htons(hello_message_to_send.msg_type);
    hello_message_to_send.chunks_amount = htons(chunks_id_read.size());
    for(uint16_t i = 0; i < chunks_id_read.size(); i++)
        hello_message_to_send.chunks_id[i] = htons(chunks_id_read[i]);

    // Enviar ao peer de contato
    int bytes_sent = sendto(client_sock, (void*)&hello_message_to_send, sizeof(HELLO_MESSAGE), 0, peer_addr->ai_addr, peer_addr->ai_addrlen);
    if (bytes_sent < 0) {
        logexit("sendto() error\n");
    }

    printf("HELLO message sent\n");

    // Aguardar a finalização de todas as threads

    for (pthread_t &thread_id : chunk_search_threads) {
        pthread_join(thread_id, NULL);
    }

    // Abrir um arquivo com o nome "output-IP.log" com o IP sendo o do cliente
    // Escrever no arquivo as associações IP e Porto com o ID do chunk obtido
    // Caso exista um chunk sem peer associado, Ip:porto = 0.0.0.0:0000
    // Salvar arquivo

    char output_file_name[32];
    char client_ip[16];

    struct sockaddr_in client_address;
    bzero(&client_address, sizeof(client_address));
    socklen_t client_address_len = sizeof(client_address);
    getpeername(client_sock, (struct sockaddr*) &client_address, &client_address_len);
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip));

    sprintf(output_file_name, "Output-%s.log", client_ip);

    std::ofstream write_file(output_file_name);

    if (!write_file.is_open()) {
        logexit("openfile() error");
    }

    for (const auto &c_info_used_in_thread : info_used_in_threads) {
        char line_to_write[64];
        char peer_ip[8];

        if (!inet_ntop(AF_INET, &(c_info_used_in_thread.second.ip_received), peer_ip, INET_ADDRSTRLEN)) {
            logexit("inet_ntop() error");
        }
        
        if (c_info_used_in_thread.second.chunk_id_associated < 0) {

        }
        sprintf(line_to_write, "%s : %d - %d\n", peer_ip, ntohs(c_info_used_in_thread.second.port_received), c_info_used_in_thread.second.chunk_id_associated);

        write_file << line_to_write;
    }

    write_file.close();

    close(client_sock);

    // Fim da execução

    return(0);
}