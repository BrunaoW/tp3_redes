#pragma once

enum MESSAGE_TYPE: uint16_t {
    HELLO,
    QUERY,
    CHUNKS_INFO,
    GET,
    RESPONSE
};

struct MESSAGE {
    MESSAGE_TYPE msg_type;
};

struct HELLO_MESSAGE : MESSAGE {
    MESSAGE_TYPE msg_type = MESSAGE_TYPE::HELLO;
    uint16_t chunks_amount = 0;
    uint16_t chunks_id[10];
};

struct GET_MESSAGE : MESSAGE {
    MESSAGE_TYPE msg_type = MESSAGE_TYPE::GET;
    uint16_t chunks_amount = 0;
    uint16_t chunks_id[10];
};

struct QUERY_MESSAGE : MESSAGE {
    MESSAGE_TYPE msg_type = MESSAGE_TYPE::QUERY;
    uint32_t client_ip;
    uint16_t client_port;
    uint16_t peer_ttl;
    uint16_t chunks_amount = 0;
    uint16_t chunks_id[10];
};

struct CHUNKS_INFO_MESSAGE : MESSAGE {
    MESSAGE_TYPE msg_type = MESSAGE_TYPE::CHUNKS_INFO;
    uint16_t chunks_amount = 0;
    uint16_t chunks_id[10];
};

struct RESPONSE_MESSAGE : MESSAGE {
    MESSAGE_TYPE msg_type = MESSAGE_TYPE::RESPONSE;
    uint16_t chunk_id;
    uint16_t chunk_size = 0;
    char chunk[1024];
};

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_in *addr_parsed);

std::string trim(std::string& str);

int parse_int(char* chars);

uint16_t parse_uint16(char* chars);

uint8_t parse_uint8(char* chars);

uint16_t parse_port_recv(char* portstr);

char* parse_ip_recv(char* ipstr);
