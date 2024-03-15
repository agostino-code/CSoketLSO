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
void alarm_handler(int signum);
void close_room();

#endif // HANDLE_ROOM_H
