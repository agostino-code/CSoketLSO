#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

typedef struct {
    int socket;
    struct sockaddr_in address;
    char* username;
    int avatar;
} Client;

#endif // CLIENT_H
