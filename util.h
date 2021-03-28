#pragma once

const uint16_t HELLO = 1;
const uint16_t QUERY = 2;
const uint16_t CHUNKS_INFO = 3;
const uint16_t GET = 4;
const uint16_t RESPONSE = 5;

struct HELLO_MESSAGE {
    uint16_t msg_type = HELLO;
    uint16_t chunks_amount = 0;
    uint16_t chunks_id[10];
};

struct GET_MESSAGE {
    uint16_t msg_type = GET;
    uint16_t chunks_amount = 0;
    uint16_t chunks_id[10];
};

struct QUERY_MESSAGE {
    uint16_t msg_type = QUERY;
    uint16_t client_ip[2];
    uint16_t client_port;
    uint16_t peer_ttl;
    uint16_t chunks_amount = 0;
    uint16_t chunks_id[10];
};

struct CHUNKS_INFO_MESSAGE {
    uint16_t msg_type = CHUNKS_INFO;
    uint16_t chunks_amount = 0;
    uint16_t chunks_id[10];
};

struct RESPONSE_MESSAGE {
    uint16_t msg_type = RESPONSE;
    uint16_t chunk_id;
    uint16_t chunk_size = 0;
    char chunk[1024];
};

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_in *addr_parsed);

std::string trim(std::string& str);

char *trimwhitespace(char *str);

int parse_int(char* chars);

uint16_t parse_uint16(char* chars);

uint8_t parse_uint8(char* chars);

uint16_t parse_port_recv(char* portstr);

char* parse_ip_recv(char* ipstr);

uint32_t parse_query_msg_ip_to_uint32(uint16_t* ip);

void parse_uint32_to_query_msg_ip(uint32_t ip_long, uint16_t* query_msg_ip);
