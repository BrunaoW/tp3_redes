#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include <string>

void logexit(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_in *addr_parsed) {
    if (addrstr == NULL || portstr == NULL) {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    struct in_addr inaddr4; // 32-bit IP address
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        addr_parsed->sin_family = AF_INET;
        addr_parsed->sin_port = port;
        addr_parsed->sin_addr = inaddr4;
        return 0;
    }

    return -1;
}

std::string trim(std::string& str) {
    str.erase(0, str.find_first_not_of(' '));       //prefixing spaces
    str.erase(str.find_last_not_of(' ')+1);         //surfixing spaces
    return str;
}

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

int powInt(int x, int y)
{
    for (int i = 0; i < y; i++)
    {
        x *= 10;
    }
    return x;
}

uint16_t parse_uint16(char* chars) {
    uint16_t sum = 0;
    int len = strlen(chars);
    for (int x = 0; x < len; x++)
    {
        int n = chars[len - (x + 1)] - '0';
        sum = sum + powInt(n, x);
    }
    return sum;
}

uint8_t parse_uint8(char* chars) {
    uint8_t sum = 0;
    int len = strlen(chars);
    for (int x = 0; x < len; x++)
    {
        int n = chars[len - (x + 1)] - '0';
        sum = sum + powInt(n, x);
    }
    return sum;
}

int parse_int(char* chars)
{
    int sum = 0;
    int len = strlen(chars);
    for (int x = 0; x < len; x++)
    {
        int n = chars[len - (x + 1)] - '0';
        sum = sum + powInt(n, x);
    }
    return sum;
}

uint16_t parse_port_recv(char* portstr) {
    uint16_t port = parse_uint16(portstr);
    port = ntohs(port);
    return(port);
}

uint32_t parse_query_msg_ip_to_uint32(uint16_t* ip) {
    uint32_t parsed_ip = ((uint32_t) ip[0] << 16) + ip[1];
    return parsed_ip;
}

void parse_uint32_to_query_msg_ip(uint32_t ip_long, uint16_t* query_msg_ip) {
    query_msg_ip[0] = (uint16_t) (ip_long >> 16);
    query_msg_ip[1] = (uint16_t) (ip_long & 0x0000FFFFuL);
}

