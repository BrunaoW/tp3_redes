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

std::string trim(std::string& str) {
    str.erase(0, str.find_first_not_of(' '));       //prefixing spaces
    str.erase(str.find_last_not_of(' ')+1);         //surfixing spaces
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
}

uint8_t parse_uint8(char* chars) {
    uint8_t sum = 0;
    int len = strlen(chars);
    for (int x = 0; x < len; x++)
    {
        int n = chars[len - (x + 1)] - '0';
        sum = sum + powInt(n, x);
    }
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

char* parse_ip_recv(char* ipstr) {
    char* parsed_ip;
    inet_ntop(AF_INET, ipstr, parsed_ip, INET_ADDRSTRLEN);

    return parsed_ip;
}
