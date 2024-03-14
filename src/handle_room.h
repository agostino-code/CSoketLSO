#ifndef HANDLE_ROOM_H
#define HANDLE_ROOM_H

#include "data/room.h"
#include <multicast.h>

// Include any necessary headers here

// Function prototypes
void *handle_room(void *arg);
void *reveal_letters(void *arg);
void finish_game(const char *username);
void start_game();
void cb(struct mcpacket *packet);

#endif // HANDLE_ROOM_H
