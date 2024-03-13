#ifndef PLAYER_H
#define PLAYER_H

#include "client.h"

enum player_status {
    SPECTATOR,
    GUESSER,
    CHOOSER
};

enum player_status getPlayerStatusFromString(const char* statusString);
const char* getPlayerStatusString(enum player_status status);

typedef struct {
    int score;
    enum player_status status;
    const Client* client;
} Player;

#endif // PLAYER_H
