#ifndef FT_PING_H
#define FT_PING_H

#include <stdlib.h>     // exit
#include <stdio.h>      // printf, fprintf, perror
#include <unistd.h>     // getpid, getuid, alarm
#include <string.h>     // memset
#include <errno.h>      // errno
#include <sys/time.h>   // gettimeofday
#include <sys/socket.h> // setsockopt, recvmsg, sendto, socket
#include <netdb.h>      // getaddrinfo
#include <arpa/inet.h>  // inet_ntop, inet_pton
#include <signal.h>     // signal
#include <netinet/ip.h> // for next header
#include <netinet/ip_icmp.h> // struct icmp, some defines

#define IP_HEADER_SIZE 20
#define ICMP_HEADER_SIZE 8
#define ICMP_DEFAULT_DATA_SIZE 56
#define DEFAULT_TTL_VALUE 118

#define NBR_OF_ARGS 5
#define ARG_BIG_G 1
#define ARG_SMALL_G 2
#define ARG_H 4
#define ARG_A 8
#define ARG_Q 16

typedef struct  args_s
{
    char        *name;
    uint8_t     value;
}               args_t;

struct data_t
{
    int socket_fd;
    unsigned int ttl;
    uint8_t args;

    int sweep_max_size;
    int sweep_min_size;
    int sweep_incr_size;

    unsigned int n_packet_send;
    unsigned int n_packet_recv;
    uint64_t min;
    uint64_t max;
    uint64_t sum;

    struct sockaddr server_addr;
    char *server_ip;
    char *server_domain;
    char *server_port;
};

uint16_t checksum(uint16_t *data, int data_len);

void print_addr(struct addrinfo addr);
void print_tab_in_hex(char *name, uint8_t *tab, int len);

#endif