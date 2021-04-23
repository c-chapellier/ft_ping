#include "ft_ping.h"

void print_addr(struct addrinfo addr)
{
    printf("addr {\n");
    printf("    flags[%d]\n", addr.ai_flags);
    printf("    family[%d]\n", addr.ai_family);
    printf("    socktype[%d]\n", addr.ai_socktype);
    printf("    protocol[%d]\n", addr.ai_protocol);
    printf("    addrlen[%d]\n", addr.ai_addrlen);

    struct sockaddr_in *addr_s = (struct sockaddr_in *)addr.ai_addr;
    printf("        addr {\n");
    printf("        family[%u]\n", addr_s->sin_family);
    printf("        port[%u]\n        addr[", ntohs(addr_s->sin_port));
    uint8_t *addr8 = (uint8_t *)&(addr_s->sin_addr.s_addr);
    for (int i = 0; i < 4; ++i)
    {
        printf("%u.", addr8[i]);
    }
    printf("]\n    }\n");

    printf("    canonname[%s]\n", addr.ai_canonname);
    printf("    next[%p]\n", addr.ai_next);
    printf("}\n");
}

void print_tab_in_hex(char *name, uint8_t *tab, int len)
{
    printf("%s: {\n", name);
    for (int i = 0; i < len; ++i)
    {
        if (i % 8 == 0)
            printf(" ");
        if (i % 16 == 0)
            printf("\n    ");
        printf("%.02x ", tab[i]);
    }
    printf("\n}\n");
}
