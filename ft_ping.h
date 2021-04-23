#ifndef FT_PING_H
#define FT_PING_H

#include <unistd.h>     // getpid, getuid, alarm
#include <sys/types.h>  // mandatory ?
#include <sys/socket.h> // getaddrinfo, setsockopt, recvmsg, sendto, socket
#include <netdb.h>      // getaddrinfo ?

#include <sys/time.h>   // gettimeofday
#include <arpa/inet.h>  // inet_ntop, inet_pton
#include <stdlib.h>     // exit
#include <signal.h>     // signal
#include <stdio.h>      // printf, fprintf ?

#include <errno.h>      // strerror
#include <string.h>     // memset

#include <netinet/ip.h> // for next header
#include <netinet/ip_icmp.h> // struct icmp, some defines

#define IP_HEADER_SIZE 20
#define ICMP_HEADER_SIZE 8
#define ICMP_DATA_SIZE 56
#define DEFAULT_TTL_VALUE 118

uint16_t checksum(uint16_t *data, int data_len);

void print_addr(struct addrinfo addr);
void print_tab_in_hex(char *name, uint8_t *tab, int len);

#endif