#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "util.h"

#define MAX_BUFFER_SIZE 1030

void usage(int argc, char **argv) {
    printf("usage: %s <ip:port> <chunk1_id>,<chunk2_id>,...,<chunkN_id>\n", argv[0]);
    printf("example: %s 127. 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_info_for_thread {
    int client_sock;
    MESSAGE_TYPE msg_received;
    in_addr ip_received;
    uint16_t port_received;
    uint16_t chunk_id_associed;
};

void* receive_chunk_info_thread(void* client_info) {
    struct client_info_for_thread* c_info_for_thread = (struct client_info_for_thread*)client_info;
    char buffer_received[MAX_BUFFER_SIZE];
    struct sockaddr_in peer_connecting_info;
    socklen_t communicator_addr_len;

    // Esperar recebimento de mensagem CHUNKS_INFO ou RESPONSE
    int bytes_received = recvfrom(c_info_for_thread->client_sock, buffer_received, strlen(buffer_received)-1, 0, (struct sockaddr*)&peer_connecting_info, &communicator_addr_len);
    if (bytes_received < 0) {
        char* error_msg;
        sprintf(error_msg, "recvfrom() error: socket = %d\n", c_info_for_thread->client_sock);

        logexit(error_msg);
    }

    char msg_type_received[2];
    msg_type_received[0] = buffer_received[0];
    msg_type_received[1] = buffer_received[1];

    uint16_t message_type_received = parse_uint16(msg_type_received);
    
    // Caso seja CHUNKS_INFO
    if (message_type_received == MESSAGE_TYPE::CHUNKS_INFO) {
        c_info_for_thread->msg_received = MESSAGE_TYPE::CHUNKS_INFO;

        // salvar ip e porto do peer que enviou
        c_info_for_thread->ip_received = peer_connecting_info.sin_addr;
        c_info_for_thread->port_received = peer_connecting_info.sin_port;

        // salvar as informações dos chunks que o peer possui
        char chunks_amount[2];
        chunks_amount[0] = buffer_received[2];
        chunks_amount[1] = buffer_received[3];

        uint16_t chunks_amount_numeric = parse_uint16(chunks_amount);
        uint16_t chunks_id_numeric[10];

        // TODO: Ver como salvar os Id's recebidos

        // - Enviar mensagem GET
        GET_MESSAGE get_message_to_send;
        get_message_to_send.chunks_amount = chunks_amount_numeric;
        memcpy(get_message_to_send.chunks_id, chunks_id_numeric, sizeof(get_message_to_send.chunks_id));

        int bytes_sent = sendto(c_info_for_thread->client_sock, &get_message_to_send, sizeof(GET_MESSAGE), 0, (struct sockaddr*)&peer_connecting_info.sin_addr, communicator_addr_len);
        if (bytes_sent < 0) {
            logexit("sendto() error\n");
        }

        pthread_exit(EXIT_SUCCESS);
    }

    // Caso seja RESPONSE
    if (message_type_received == MESSAGE_TYPE::RESPONSE) {
        c_info_for_thread->msg_received = MESSAGE_TYPE::RESPONSE;

        // Identificar chunk recebido
        char chunk_received_id[2];
        chunk_received_id[0] = buffer_received[3];
        chunk_received_id[1] = buffer_received[4];

        uint16_t chunk_received_id_numeric = parse_uint16(chunk_received_id);
        c_info_for_thread->chunk_id_associed = chunk_received_id_numeric;
        
        // - Abrir um arquivo para escrita
        char* file_name;
        sprintf(file_name, "received_chunk_%d", chunk_received_id_numeric);

        char chunk_content_char_array[1024];
        strncpy(chunk_content_char_array, buffer_received + 7, 1024);

        // - Escrever o buffer recebido no arquivo
        std::ofstream write_file(file_name);
        
        if(!write_file.is_open()) {
            logexit("open file error\n");
        }

        write_file.write(chunk_content_char_array, sizeof(char) * 1024);

        // - salvar arquivo com o respectivo nome do chunk
        write_file.close();
        
        // - salvar ip e porto do peer que enviou
        c_info_for_thread->ip_received = peer_connecting_info.sin_addr;
        c_info_for_thread->port_received = peer_connecting_info.sin_port;

        // - encerrar thread
        pthread_exit(EXIT_SUCCESS);
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

    // Criar N threads, de acordo com a quantidade de chunks requisitados

    std::vector<pthread_t> chunk_search_threads;
    std::vector<struct client_info_for_thread*> info_used_in_threads;

    for (uint16_t &chunk_id : chunks_id_read) {
        // Criar socket UDP
        int client_sock;
        client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (client_sock < 0) {
            logexit("socket() error\n");
        }

        struct client_info_for_thread c_info_for_thread;
        struct client_info_for_thread alternate_c_info_for_thread;

        c_info_for_thread.client_sock = client_sock;
        alternate_c_info_for_thread.client_sock = client_sock;

        pthread_t chunk_thread_id;
        if (pthread_create(&chunk_thread_id, NULL, &receive_chunk_info_thread, &c_info_for_thread) != 0) {
            logexit("pthread_create() error\n");
        }

        pthread_t alternate_chunk_thread_id;
        if (pthread_create(&alternate_chunk_thread_id, NULL, &receive_chunk_info_thread, &alternate_c_info_for_thread) != 0) {
            logexit("pthread_create() error\n");
        }

        chunk_search_threads.push_back(chunk_thread_id);
        chunk_search_threads.push_back(alternate_chunk_thread_id);

        info_used_in_threads.push_back(&c_info_for_thread);
        info_used_in_threads.push_back(&alternate_c_info_for_thread);
    }

    // Essas threads irão esperar, por um tempo determinado, um recvfrom dos peers que contém os chunks
    // caso ultrapasse esse tempo de espera, será dado um timeout

    // Montar uma HELLO_MESSAGE
    uint16_t *chunks_id_array = &chunks_id_read[0];

    HELLO_MESSAGE hello_message_to_send;
    hello_message_to_send.chunks_amount = chunks_id_read.size();
    memcpy(hello_message_to_send.chunks_id, chunks_id_array, hello_message_to_send.chunks_amount);

    // Cria socket pra mensagem HELLO    
    int hello_msg_client_sock;
    hello_msg_client_sock = socket(peer_addr->ai_family, peer_addr->ai_socktype, peer_addr->ai_protocol);
    if (hello_msg_client_sock < 0) {
        logexit("socket() error\n");
    }

    // Enviar ao peer de contato
    int bytes_sent = sendto(hello_msg_client_sock, &hello_message_to_send, sizeof(HELLO_MESSAGE), 0, peer_addr->ai_addr, peer_addr->ai_addrlen);
    if (bytes_sent < 0) {
        logexit("sendto() error\n");
    }

    // Aguardar a finalização de todas as threads

    for (pthread_t &thread_id : chunk_search_threads) {
        pthread_join(thread_id, NULL);
    }

    // Abrir um arquivo com o nome "output-IP.log" com o IP sendo o do cliente
    // Escrever no arquivo as associações IP e Porto com o ID do chunk obtido
    // Caso exista um chunk sem peer associado, Ip:porto = 0.0.0.0:0000
    // Salvar arquivo

    char *output_file_name;
    char *client_ip;

    inet_ntop(AF_INET, contact_peer_info.ai_addr, client_ip, INET_ADDRSTRLEN);

    sprintf(output_file_name, "Output-%s.log", client_ip);

    std::ofstream write_file(output_file_name);

    if (!write_file.is_open()) {
        logexit("openfile() error");
    }

    for (struct client_info_for_thread* &c_info_used_in_thread : info_used_in_threads) {
        if (c_info_used_in_thread->msg_received == MESSAGE_TYPE::RESPONSE) {
                char *line_to_write;
                char *peer_ip;

                inet_ntop(AF_INET, &(c_info_used_in_thread->ip_received), peer_ip, INET_ADDRSTRLEN);
                
                sprintf(line_to_write, "%s : %d - %d\n", peer_ip, c_info_used_in_thread->port_received, c_info_used_in_thread->chunk_id_associed);

                write_file << line_to_write;
        }
    }

    write_file.close();

    // Fim da execução

    return(0);
}