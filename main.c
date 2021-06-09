#include "ft_ping.h"

static void fill_args(char *argv[]);
static void get_ips();
static void create_socket();
static void pinger(int signum);
static void ping();
static void catcher();
static void end(int signum);

struct data_t metadata;

int main(int argc, char *argv[])
{
    char ip_buffer[INET_ADDRSTRLEN];

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s [-vh] host\n", argv[0]);
        fprintf(stderr, "       %s [-vh] mcast-group\n", argv[0]);
        exit(1);
    }           

    memset(ip_buffer, 0, sizeof ip_buffer);

    metadata.ttl = DEFAULT_TTL_VALUE;
    metadata.args = 0;

    metadata.sweep_min_size = 8;    // will be 0
    metadata.sweep_incr_size = 1;

    metadata.n_packet_send = 0;
    metadata.n_packet_recv = 0;
    metadata.min = UINT64_MAX;
    metadata.max = 0;
    metadata.sum = 0;

    metadata.server_domain = argv[argc - 1];
    metadata.server_ip = ip_buffer;
    metadata.server_port = "http";

    fill_args(&argv[1]);
    if (metadata.args & ARG_BIG_G)
    {
        if (metadata.sweep_min_size < 8)
        {
            fprintf(stderr, "must be > 8\n");
            exit(1);
        }
        if (metadata.sweep_incr_size < 0)
        {
            fprintf(stderr, "must be >= 0\n");
            exit(1);
        }
        if (metadata.sweep_min_size >= metadata.sweep_max_size)
        {
            fprintf(stderr, "ping: Maximum packet size must be greater than the minimum packet size\n");
            exit(1);
        }
    }
    else
    {
        if (metadata.args & ARG_SMALL_G || metadata.args & ARG_H)
        {
            fprintf(stderr, "h or g must be with G\n");
            exit(1);
        }
    }
    
    // check args
    // printf("%d %d %d\n", metadata.sweep_max_size, metadata.sweep_min_size, metadata.sweep_incr_size);

    get_ips();
    create_socket();

    printf("FT_PING %s (%s): ",  metadata.server_domain, metadata.server_ip);
    if (metadata.args & ARG_BIG_G)
    {
        printf("(%d ... %d)", metadata.sweep_min_size, metadata.sweep_max_size);
    }
    else
    {
        printf("%u", ICMP_DEFAULT_DATA_SIZE);
    }
    printf(" data bytes\n");

	signal(SIGINT, end);
    signal(SIGALRM, pinger);

    ping();
    alarm(1);

    catcher();
}

static void fill_args(char *argv[])
{
    args_t  args[NBR_OF_ARGS];

    args[0].name = "-G";
    args[0].value = ARG_BIG_G;
    args[1].name = "-g";
    args[1].value = ARG_SMALL_G;
    args[2].name = "-h";
    args[2].value = ARG_H;
    args[3].name = "-a";
    args[3].value = ARG_A;
    args[4].name = "-q";
    args[4].value = ARG_Q;
    for (int i = 0; argv[i] != NULL; ++i)
    {
        for (int j = 0; j < NBR_OF_ARGS; ++j)
        {
            if (strcmp(argv[i], args[j].name) == 0)
            {
                metadata.args |= args[j].value;
                switch (args[j].value)
                {
                    case ARG_BIG_G:
                        metadata.sweep_max_size = atoi(argv[i + 1]);
                        break ;
                    case ARG_SMALL_G:
                        metadata.sweep_min_size = atoi(argv[i + 1]);
                        break ;
                    case ARG_H:
                        metadata.sweep_incr_size = atoi(argv[i + 1]);
                        break ;
                }
            }
        }
    }
}

static void get_ips()
{
    struct addrinfo hints, *res;
    int rc;
    const char *prc;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_ICMP;

#if __APPLE__
    rc = getaddrinfo(metadata.server_domain, metadata.server_port, &hints, &res);
#elif __linux__
    rc = getaddrinfo(metadata.server_domain, NULL, &hints, &res);
#endif
    if (rc != 0)
    {
        fprintf(stderr, "ft_ping: cannot resolve %s: unknow host\n", metadata.server_domain);
        exit(1);
    }

    // only check the first elem of the res list
    metadata.server_addr = *(res->ai_addr);
    
    prc = inet_ntop(AF_INET, &((struct sockaddr_in *)&metadata.server_addr)->sin_addr, metadata.server_ip, INET_ADDRSTRLEN);
    if (prc == NULL)
    {
        perror("ping: inet_ntop");
        exit(1);
    }
}

static void create_socket()
{
    struct timeval tv;

    metadata.socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (metadata.socket_fd == -1)
    {
        perror("ping: socket");
        exit(1);
    }

    setsockopt(metadata.socket_fd, IPPROTO_IP, IP_TTL, &metadata.ttl, sizeof metadata.ttl);

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(metadata.socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
}

static void pinger(int signum)
{
    (void)signum;

    ping();
	signal(SIGALRM, pinger);
    alarm(1);
}

static void ping()
{
    int icmp_data_size = ICMP_DEFAULT_DATA_SIZE;

    if (metadata.args & ARG_BIG_G)
    {
        icmp_data_size = metadata.sweep_min_size + metadata.n_packet_send * metadata.sweep_incr_size;
        if (icmp_data_size > metadata.sweep_max_size)
        {
            end(0);
        }
    }
    
    uint8_t outpack[ICMP_HEADER_SIZE + icmp_data_size];
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
    ping_request->icmp_seq = metadata.n_packet_send++;

    timestamp = (uint32_t *)&outpack[8];
    gettimeofday(&tv, NULL);
    timestamp[0] = htonl(tv.tv_sec);
    timestamp[1] = htonl(tv.tv_usec);

    for (int i = 16; i < (int)sizeof outpack; ++i)
    {
        outpack[i] = i - 8;
    }

    // printf("%p\n", outpack);
    // printf("%p\n", &outpack);
    ping_request->icmp_cksum = checksum((uint16_t *)outpack, sizeof outpack);

    // print_tab_in_hex("echo_request", outpack, sizeof outpack);
    rc = sendto(metadata.socket_fd, outpack, sizeof outpack, 0, &metadata.server_addr, sizeof metadata.server_addr);
    if (rc == -1)
    {
        perror("ping: sendto");
    }
    if (!(metadata.args & ARG_Q) && metadata.args & ARG_A)
    {
        printf("** beep **\n\a");
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
        rc = recvmsg(metadata.socket_fd, &msg, 0);
        if (rc < 0)
        {
            switch (errno)
            {
                case EAGAIN:
                    printf("Request timeout for icmp_seq %d\n", metadata.n_packet_send - 1);
                    break ;
#if __linux__
                case EINTR:
                    break ;
#endif
                default:
                    printf("ret = %d\n", errno);
                    perror("ping: recvmsg");
                    exit(1);
            }
            continue ;
	    }

        gettimeofday(&tv_recv, NULL);
        ping_reply = (struct icmp *)&msg_buffer[20];

        /* if response is valid */
        if (ping_reply->icmp_type == ICMP_ECHOREPLY && ping_reply->icmp_id == getpid())
        {
            uint32_t *timestamp;
            struct timeval tv_send;
            uint64_t diff;

            ++metadata.n_packet_recv;

            timestamp = (uint32_t *)&msg_buffer[28];
            tv_send.tv_sec = ntohl(timestamp[0]);
            tv_send.tv_usec = ntohl(timestamp[1]);

            diff = (tv_recv.tv_sec - tv_send.tv_sec) * UINT32_MAX + (tv_recv.tv_usec - tv_send.tv_usec);
            if (diff < metadata.min)
                metadata.min = diff;
            if (diff > metadata.max)
                metadata.max = diff;
            metadata.sum += diff;

            if (!(metadata.args & ARG_Q))
            {
                printf("%u bytes from %s: icmp_seq=%u ttl=%u time=%.03f ms\n", rc - IP_HEADER_SIZE, metadata.server_ip, ping_reply->icmp_seq, metadata.ttl, diff / 1000.0);
            }
        }
    }
}

static void end(int signum)
{
    double packet_loss;
    double fmin, fmax, fsum, fmean;

    (void)signum;

    printf("\n--- %s ft_ping statistics ---\n", metadata.server_domain);

    if (metadata.n_packet_send != 0)
        packet_loss = (metadata.n_packet_send - metadata.n_packet_recv) / metadata.n_packet_send;
    else
        packet_loss = 1.0;
    printf("%u packets transmitted, %u packets received, %.01f%% packet loss\n", metadata.n_packet_send, metadata.n_packet_recv, packet_loss * 100.0);

    if (metadata.n_packet_recv > 0)
    {
        fmin = metadata.min / 1000.0;
        fmax = metadata.max / 1000.0;
        fsum = metadata.sum / 1000.0;
        fmean = fsum / metadata.n_packet_recv;
        printf("round-trip min/avg/max/stddev = %.03f/%.03f/%.03f/? ms\n", fmin, fmean, fmax);
    }

    exit(0);
}