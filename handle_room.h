#ifndef HANDLE_ROOM_H
#define HANDLE_ROOM_H

#include "data/room.h"
// Include any necessary headers here

// Function prototypes
void *handle_room(void *arg);

void broadcast_to_room(const Room *room, const char *message);



#endif // HANDLE_ROOM_H
