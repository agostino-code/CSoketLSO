#include <stdio.h>
#include <sys/socket.h>
#include "handle_player.h"

void* handle_player(void* arg) {
    const int client_socket = *(int *) arg;
    char request[1024];
    ssize_t num_bytes;

    // Receive and process client requests
    while ((num_bytes = recv(client_socket, request, sizeof(request) - 1, 0)) > 0) {
        request[num_bytes] = '\0';
        printf("Received request: %s\n", request);

        // Extract the request type
        char requestType[256];
        //if request type is message brodcast to all client

        //if request type is quit brodcast to all client your leave

        //if request type is roomlist of all rooms get all room data
    }
    return 0;
}