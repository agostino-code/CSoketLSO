#include "globals.h"

Room rooms[MAX_ROOMS];
int num_rooms=0;
// Array to store connected clients
Client clients[MAX_CLIENTS];
int num_clients=0;

PGconn* conn;
// Add your code here
// Mutex for synchronizing access to the clients array
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
//Mutex for synchronizing access to the rooms array
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;

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