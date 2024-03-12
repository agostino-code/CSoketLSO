#include "player.h"
#include <string.h>

enum player_status getPlayerStatusFromString(const char* statusString) {
    if (strcmp(statusString, "SPECTATOR") == 0) {
        return SPECTATOR;
    } else if (strcmp(statusString, "GUESSER") == 0) {
        return GUESSER;
    } else if (strcmp(statusString, "CHOOSER") == 0) {
        return CHOOSER;
    } else {
        // Handle unknown status or return a default value
        return SPECTATOR; // You can choose a different default if needed
    }
}

const char* getPlayerStatusString(enum player_status status) {
    switch (status) {
        case SPECTATOR:
            return "SPECTATOR";
        case GUESSER:
            return "GUESSER";
        case CHOOSER:
            return "CHOOSER";
        default:
            // Handle unknown status or return a default value
            return "SPECTATOR"; // You can choose a different default if needed
    }
}