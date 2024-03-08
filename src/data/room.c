#include "room.h"

enum language getLanguageFromString(const char* langString) {
    if (strcmp(langString, "en") == 0) {
        return ENGLISH;
    } else if (strcmp(langString, "it") == 0) {
        return ITALIAN;
    } else if (strcmp(langString, "es") == 0) {
        return SPANISH;
    } else if (strcmp(langString, "de") == 0) {
        return GERMAN;
    } else {
        // Handle unknown language or return a default value
        return ENGLISH; // You can choose a different default if needed
    }
}

enum max_player getMaxPlayerFromNumber(int number) {
    switch (number) {
        case 10:
            return TEN;
        case 15:
            return FIFTEEN;
        case 20:
            return TWENTY;
        default:
            // Return a default maxplayer or handle the error case
            // Here, we return AVATAR_1 as the default
            return TEN;
    }
}