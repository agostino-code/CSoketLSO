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
    Room *room = (Room *)arg;
    /* Create a datagram socket on which to receive. */
    int sock;
    struct sockaddr_in addr;
    struct ip_mreq join_addr;
    char buf[1024];
    int str_len;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(room->port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        error_handling("bind() error");

    join_addr.imr_multiaddr.s_addr = inet_addr(GROUP);
    join_addr.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_addr, sizeof(join_addr)) == -1)
        error_handling("setsockopt() error");

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    while (1)
    {
        str_len = recvfrom(sock, buf, BUF_SIZE - 1, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (str_len < 0)
        {
            perror("recvfrom() error");
            break;
        }
        if (str_len > 0)
            buf[str_len] = '\0';

        printf("Received message: %s\n", buf);

        // Send benvenuto to client
        char *welcome = "Benvenuto";
        sendto(sock, welcome, strlen(welcome), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    }

    // Leave multicast group
    setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *)&join_addr, sizeof(join_addr));

    // Close the socket
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    perror(message);
    exit(1);
}