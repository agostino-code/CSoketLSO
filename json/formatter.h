#ifndef FORMATTER_H
#define FORMATTER_H

#include <json.h>
#include "../data/user.h"
#include "../data/room.h"
#include "../data/player.h"
#include "../data/request.h"

const char *userToJson(const User *userObj);

// userParse
User *userParse(json_object *root);
// Send all room information to client
const char *roomToJson(const Room *roomObj);
// roomParse
Room *roomParse(json_object *root);

// Function for creating a JSON error message
const char *createJsonErrorMessage(const char *errorMessage);

// Function for creating a JSON success message
const char *createJsonSuccessMessage(const char *successMessage);

const char *createJsonListOfRooms();

// string to REQUEST
Request *parseRequest(const char *string);

#endif
