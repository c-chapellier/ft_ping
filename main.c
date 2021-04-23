#include "ft_ping.h"

int socket_fd, n_packet_send = 0, n_packet_recv = 0;
uint64_t min = UINT64_MAX, max = 0, sum = 0;
struct sockaddr server_addr;
char *server_ip;
char *server_domain, *server_port;

int ttl = DEFAULT_TTL_VALUE;

static void get_ips()
{
    struct addrinfo hints, *res;
    int rc;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_ICMP;

    rc = getaddrinfo(server_domain, server_port, &hints, &res);
    if (rc != 0)
    {
        fprintf(stderr, "error: ft_ping: getaddrinfo: %s\n", gai_strerror(rc));
        exit(1);
    }

    // only check the first elem of the res list
    server_addr = *(res->ai_addr);
    server_ip = inet_ntoa(((struct sockaddr_in *)&server_addr)->sin_addr);
    // print_addr(*res);
}

static void create_socket()
{
    socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (socket_fd == -1)
    {
        fprintf(stderr, "error: ft_ping: socket: %s\n", strerror(errno));
        exit(1);
    }

    setsockopt(socket_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof ttl);

    // int packlen = 256;
	// setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &packlen, sizeof(packlen));
}

static void ping()
{
    uint8_t outpack[ICMP_HEADER_SIZE + ICMP_DATA_SIZE];
    struct icmp *ping_request;
    uint32_t *timestamp;
    struct timeval tv;
    int rc;

    memset(&outpack, 0, sizeof outpack);

    ping_request = (struct icmp *)outpack;
    ping_request->icmp_type = ICMP_ECHO;
    ping_request->icmp_code = 0;
    ping_request->icmp_cksum = 0;
    ping_request->icmp_id = getpid();
    ping_request->icmp_seq = n_packet_send++;

    timestamp = (uint32_t *)&outpack[8];
    gettimeofday(&tv, NULL);
    timestamp[0] = htonl(tv.tv_sec);
    timestamp[1] = htonl(tv.tv_usec);

    for (int i = 16; i < (int)sizeof outpack; ++i)
    {
        outpack[i] = i - 8;
    }

    ping_request->icmp_cksum = checksum((uint16_t *)&outpack, sizeof outpack);

    // print_tab_in_hex("echo_request", outpack, sizeof outpack);

    rc = sendto(socket_fd, &outpack, sizeof outpack, 0, &server_addr, sizeof server_addr);
    if (rc == -1)
    {
        fprintf(stderr, "error: ft_ping: sendto: %s\n", strerror(errno));
        exit(1);
    }
}

static void catcher()
{
    struct msghdr msg;
    struct iovec iov[1];
    uint8_t msg_buffer[256];
    register int rc;
    struct timeval tv_recv;
    struct icmp *ping_reply;

    memset(&msg, 0, sizeof msg);
    memset(iov, 0, sizeof iov);
    memset(&msg_buffer, 0, sizeof msg_buffer);

    iov[0].iov_base = msg_buffer;
    iov[0].iov_len = sizeof msg_buffer;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    while (1)
    {
        rc = recvmsg(socket_fd, &msg, 0);
        if (rc < 0)
        {
            fprintf(stderr, "error: ft_ping: read: %s\n", strerror(errno));
            exit(1);
	    }

        // print_tab_in_hex("msg_buffer", msg_buffer, sizeof msg_buffer);
        // printf("cmsg_len[%u]\n", control_buffer.cmsg_len);
        // printf("cmsg_level[%d]\n", control_buffer.cmsg_level);
        // printf("cmsg_type[%d]\n", control_buffer.cmsg_type);
        // print_tab_in_hex("msg_buffer", control_buffer.cmsg_data, sizeof control_buffer.cmsg_data);
        // printf("flags[%d]\n", msg.msg_flags);

        gettimeofday(&tv_recv, NULL);
        ping_reply = (struct icmp *)&msg_buffer[20];

        /* if response is valid */
        if (ping_reply->icmp_type == ICMP_ECHOREPLY && ping_reply->icmp_id == getpid())
        {
            uint32_t *timestamp;
            struct timeval tv_send;
            uint64_t diff;

            ++n_packet_recv;

            timestamp = (uint32_t *)&msg_buffer[28];
            tv_send.tv_sec = ntohl(timestamp[0]);
            tv_send.tv_usec = ntohl(timestamp[1]);

            diff = (tv_recv.tv_sec - tv_send.tv_sec) * UINT32_MAX + (tv_recv.tv_usec - tv_send.tv_usec);
            if (diff < min)
                min = diff;
            if (diff > max)
                max = diff;
            sum += diff;

            printf("%u bytes from %s: icmp_seq=%u ttl=%u time=%.03f ms\n", rc - IP_HEADER_SIZE, server_ip, ping_reply->icmp_seq, ttl, diff / 1000.0);
        }
    }
}

static void pinger(int signum)
{
    (void)signum;

    ping();
	signal(SIGALRM, pinger);
    alarm(1);
}

static void end(int signum)
{
    double packet_loss;
    double fmin, fmax, fsum, fmean;

    (void)signum;

    printf("\n--- %s ping statistics ---\n", server_domain);

    packet_loss = (n_packet_send - n_packet_recv) / n_packet_recv;
    printf("%u packets transmitted, %u packets received, %.01f%% packet loss\n", n_packet_send, n_packet_recv, packet_loss);

    fmin = min / 1000.0;
    fmax = max / 1000.0;
    fsum = sum / 1000.0;
    fmean = fsum / n_packet_recv;
    printf("round-trip min/avg/max/stddev = %.03f/%.03f/%.03f/? ms\n", fmin, fmean, fmax);

    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s [-vh] host\n", argv[0]);
        fprintf(stderr, "       %s [-vh] mcast-group\n", argv[0]);
        exit(1);
    }           

    server_domain = argv[1];
    server_port = "http";
    get_ips();
    create_socket();

    printf("PING %s (%s): %u data bytes\n", server_domain, server_ip, ICMP_DATA_SIZE);

	signal(SIGINT, end);
    signal(SIGALRM, pinger);

    ping();
    alarm(1);

    catcher();
}
