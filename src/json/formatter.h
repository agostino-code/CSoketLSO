#ifndef FORMATTER_H
#define FORMATTER_H

#include <ctype.h>
#include "../data/user.h"
#include "../data/room.h"
#include "../data/player.h"
#include "../data/request.h"
#include "../data/response.h"

const char *userToJson(const User *userObj);


User *userParse(json_object *root);

const char* roomToJson(Room* room);

Room *roomParse(json_object *root);

const char *createJsonErrorMessage(const char *errorMessage);

const char *createJsonSuccessMessage(const char *successMessage);

const char *createJsonListOfRooms();

Request *parseRequest(const char *string);

Response *parseResponse(const char *string);

const char *createJsonNotification(const char *whatHappened, const char *username);

#endif
