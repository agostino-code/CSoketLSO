#include "player.h"
#include <string.h>

enum player_status getPlayerStatusFromString(const char* statusString) {
    if (strcmp(statusString, "spectator") == 0) {
        return SPECTATOR;
    } else if (strcmp(statusString, "guesser") == 0) {
        return GUESSER;
    } else if (strcmp(statusString, "chooser") == 0) {
        return CHOOSER;
    } else {
        // Handle unknown status or return a default value
        return SPECTATOR; // You can choose a different default if needed
    }
}

const char* getPlayerStatusString(enum player_status status) {
    switch (status) {
        case SPECTATOR:
            return "spectator";
        case GUESSER:
            return "guesser";
        case CHOOSER:
            return "chooser";
        default:
            // Handle unknown status or return a default value
            return "spectator"; // You can choose a different default if needed
    }
}