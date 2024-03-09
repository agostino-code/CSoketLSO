#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#include "handle_room.h"
#include "handle_player.h"
#include "globals.h"

#define GROUP "228.5.6.7"
#define BUF_SIZE 1024
void *handle_room(void *arg)
{
fprintf(stderr, "handle_room\n");

    Room *room = (Room *)arg;
    /* Create a datagram socket on which to receive. */
    int sock;
    struct sockaddr_in addr;
    struct ip_mreq join_addr;
    char buf[1024];
    int str_len, addr_len;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(room->port);
    fprintf(stderr, "Multicast on the port: %d\n", addr.sin_port);
    addr_len = sizeof(addr);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        error_handling("bind() error");

    join_addr.imr_multiaddr.s_addr = inet_addr(GROUP);
    join_addr.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_addr, sizeof(join_addr)) == -1)
        error_handling("setsockopt() error");


    while (1)
    {
        str_len = recvfrom(sock, buf, BUF_SIZE - 1, 0, (struct sockaddr *)&addr, &addr_len);
        if (str_len < 0)
        {
            perror("recvfrom() error");
            break;
        }
        if (str_len > 0)
            buf[str_len] = '\0';

        fprintf(stderr, "Received message: %s\n", buf);
    }

    // Close the socket
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}