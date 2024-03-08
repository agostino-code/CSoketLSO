#ifndef GLOBALS_H
#define GLOBALS_H

#include <libpq-fe.h>
#include "data/client.h"
#include "data/room.h"
#include "config.h"

// Add your global variable declarations here
extern Room rooms[];
extern int num_rooms;
// Array to store connected clients
extern Client clients[];
extern int num_clients;

extern PGconn* conn;
// Add your code here
// Mutex for synchronizing access to the clients array
extern pthread_mutex_t clients_mutex;
//Mutex for synchronizing access to the rooms array
extern pthread_mutex_t rooms_mutex;

Client* find_client_by_socket(int socket);

#endif // GLOBALS_H
