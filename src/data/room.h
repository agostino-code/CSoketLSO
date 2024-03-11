#ifndef ROOM_H
#define ROOM_H

#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
 
#include "player.h"
#include "../config.h"

enum max_player {
    TEN = 10,
    FIFTEEN = 15,
    TWENTY = 20
};

enum max_player getMaxPlayerFromNumber(int number);

enum language {
    ENGLISH,
    ITALIAN,
    SPANISH,
    GERMAN
};

enum language getLanguageFromString(const char* langString);

typedef struct {
    char* name;
    unsigned long numberOfPlayers;
    enum max_player maxPlayers;
    int round;
    enum language language;
    Player* players[MAX_PLAYERS];
    char* address;
    pthread_t thread;
    bool inGame;
    const char* word;
    const char* mixedletters;
    char* revealedletters;
} Room;

#endif // ROOM_H
