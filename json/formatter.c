#include "formatter.h"
#include "../globals.h"

const char *userToJson(const User *userObj)
{
    //{"responseType":"SUCCESS","data":{"email":"agostino.cesarano@gmail.com","password":"aaaaaa","username":null,"avatar":null}}
    json_object *root = json_object_new_object();

    json_object *data = json_object_new_object();
    json_object_object_add(root, "responseType", json_object_new_string("SUCCESS"));
    json_object_object_add(root, "data", data);
    json_object_object_add(data, "email", json_object_new_string(userObj->email));
    json_object_object_add(data, "username", json_object_new_string(userObj->username));
    json_object_object_add(data, "password", json_object_new_string(userObj->password));
    json_object_object_add(data, "avatar", json_object_new_int(userObj->avatar));

    const char *json = json_object_to_json_string(root);
    // json_object_put(root);
    return json;
}

// userParse
User *userParse(json_object *root)
{
    User *user = (User *)malloc(sizeof(User));
    user->email = (char *)json_object_get_string(json_object_object_get(root, "email"));
    user->username = (char *)json_object_get_string(json_object_object_get(root, "username"));
    user->password = (char *)json_object_get_string(json_object_object_get(root, "password"));
    user->avatar = json_object_get_int(json_object_object_get(root, "avatar"));
    return user;
}

// Send all room information to client
const char *roomToJson(const Room *roomObj)
{
    json_object *root = json_object_new_object();
    json_object *players = json_object_new_array();
    json_object *data = json_object_new_object();
    json_object_object_add(root, "responseType", json_object_new_string("SUCCESS"));
    json_object_object_add(root, "data", data);
    json_object_object_add(data, "name", json_object_new_string(roomObj->name));
    json_object_object_add(data, "numberOfPlayers", json_object_new_int(roomObj->numberOfPlayers));
    json_object_object_add(data, "maxNumberOfPlayers", json_object_new_int(roomObj->maxPlayers));
    json_object_object_add(data, "port", json_object_new_int(roomObj->address.sin_port));
    json_object_object_add(data, "round", json_object_new_int(roomObj->round));
    json_object_object_add(data, "players", players);
    for (int i = 0; i < roomObj->numberOfPlayers; i++)
    {
        json_object *player = json_object_new_object();
        json_object_object_add(player, "username", json_object_new_string(roomObj->players[i].client.username));
        json_object_object_add(player, "avatar", json_object_new_int(roomObj->players[i].client.avatar));
        json_object_object_add(player, "score", json_object_new_int(roomObj->players[i].score));
        json_object_object_add(player, "status", json_object_new_string(getPlayerStatusString(roomObj->players[i].status)));
        json_object_array_add(players, player);
    }
    switch (roomObj->language)
    {
    case ENGLISH:
        json_object_object_add(data, "language", json_object_new_string("en"));
        break;
    case ITALIAN:
        json_object_object_add(data, "language", json_object_new_string("it"));
        break;
    case SPANISH:
        json_object_object_add(data, "language", json_object_new_string("es"));
        break;
    case GERMAN:
        json_object_object_add(data, "language", json_object_new_string("de"));
        break;
    }
    const char *json = json_object_to_json_string(root);
    json_object_put(root);
    return json;
}

// roomParse
Room *roomParse(json_object *root)
{
    Room *room = (Room *)malloc(sizeof(Room));
    room->name = (char *)json_object_get_string(json_object_object_get(root, "name"));
    room->numberOfPlayers = json_object_get_int(json_object_object_get(root, "numberOfPlayers"));
    room->maxPlayers = json_object_get_int(json_object_object_get(root, "maxNumberOfPlayers"));
    room->round = json_object_get_int(json_object_object_get(root, "round"));
    room->language = getLanguageFromString(json_object_get_string(json_object_object_get(root, "language")));
    json_object *players = json_object_object_get(root, "players");
    for (int i = 0; i < room->numberOfPlayers; i++)
    {
        json_object *player = json_object_array_get_idx(players, i);
        room->players[i].client.username = (char *)json_object_get_string(json_object_object_get(player, "username"));
        room->players[i].client.avatar = json_object_get_int(json_object_object_get(player, "avatar"));
        room->players[i].score = json_object_get_int(json_object_object_get(player, "score"));
        room->players[i].status = getPlayerStatusFromString(json_object_get_string(json_object_object_get(player, "status")));
    }
    return room;
}

// Function for creating a JSON error message
const char *createJsonErrorMessage(const char *errorMessage)
{
    json_object *root = json_object_new_object();
    json_object_object_add(root, "responseType", json_object_new_string("ERROR"));
    json_object_object_add(root, "data", json_object_new_string(errorMessage));
    const char *json = json_object_to_json_string(root);
    // json_object_put(root);
    return json;
}

// Function for creating a JSON success message
const char *createJsonSuccessMessage(const char *successMessage)
{
    json_object *root = json_object_new_object();
    json_object_object_add(root, "responseType", json_object_new_string("SUCCESS"));
    json_object_object_add(root, "data", json_object_new_string(successMessage));
    const char *json = json_object_to_json_string(root);
    // json_object_put(root);
    return json;
}

const char *createJsonListOfRooms()
{
    pthread_mutex_lock(&rooms_mutex);
    json_object *root = json_object_new_object();
    json_object *array = json_object_new_array();
    json_object_object_add(root, "responseType", json_object_new_string("SUCCESS"));
    json_object_object_add(root, "data", array);
    for (int i = 0; i < num_rooms; i++)
    {
        json_object *room = json_object_new_object();
        json_object_object_add(room, "name", json_object_new_string(rooms[i].name));
        json_object_object_add(room, "numberOfPlayers", json_object_new_int(rooms[i].numberOfPlayers));
        json_object_object_add(room, "maxNumberOfPlayers", json_object_new_int(rooms[i].maxPlayers));
        json_object_object_add(room, "port", json_object_new_int(rooms[i].address.sin_port));
        json_object_array_add(array, room);
    }
    const char *json = json_object_to_json_string(root);
    pthread_mutex_unlock(&rooms_mutex);
    return json;
}

// string to REQUEST
Request *parseRequest(const char *string)
{
    Request *request = (Request *)malloc(sizeof(Request));
    json_object *root = json_tokener_parse(string);
    request->type = json_object_get_string(json_object_object_get(root, "requestType"));
    request->data = json_object_object_get(root, "data");
    return request;
}