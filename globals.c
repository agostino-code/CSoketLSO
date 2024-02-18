#include "globals.h"

Client* find_client_by_socket(int socket)
{
    for (int i = 0; i < num_clients; i++)
    {
        if (clients[i].socket == socket)
        {
            return &clients[i];
        }
    }
    return NULL;
}