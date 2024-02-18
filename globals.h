#ifndef GLOBALS_H
#define GLOBALS_H

#include <libpq-fe.h>
#include "data/client.h"
#include "data/room.h"

// Add your global variable declarations here
static Room rooms[MAX_ROOMS];
static int num_rooms = 0;
// Array to store connected clients
static Client clients[MAX_CLIENTS];
static int num_clients=0;

static PGconn* conn;
// Add your code here
// Mutex for synchronizing access to the clients array
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
//Mutex for synchronizing access to the rooms array
static pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;

Client* find_client_by_socket(int socket);

#endif // GLOBALS_H
