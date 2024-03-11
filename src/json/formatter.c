#include "formatter.h"
#include "../globals.h"
#include <ctype.h> 

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
    const char *email = json_object_get_string(json_object_object_get(root, "email"));
    int length = strlen(email);
    char *lowercaseEmail = (char *)malloc(length + 1);
    for (int i = 0; i < length; i++) {
        lowercaseEmail[i] = tolower(email[i]);
    }
    lowercaseEmail[length] = '\0';
    user->email = lowercaseEmail;
    user->username = (char *)json_object_get_string(json_object_object_get(root, "username"));
    user->password = (char *)json_object_get_string(json_object_object_get(root, "password"));
    user->avatar = json_object_get_int(json_object_object_get(root, "avatar"));
    return user;
}

// Send all room information to client
const char* roomToJson(Room* room)
{
    json_object *root = json_object_new_object();
    json_object *players = json_object_new_array();
    json_object *data = json_object_new_object();
    json_object_object_add(root, "responseType", json_object_new_string("SUCCESS"));
    json_object_object_add(root, "data", data);
    json_object_object_add(data, "name", json_object_new_string(room->name));
    json_object_object_add(data, "numberOfPlayers", json_object_new_int(room->numberOfPlayers));
    json_object_object_add(data, "maxNumberOfPlayers", json_object_new_int(room->maxPlayers));
    if(room->word != NULL)
        json_object_object_add(data, "word", json_object_new_string(room->word));
    if(room->chooser != NULL)
        json_object_object_add(data, "chooser", json_object_new_string(room->chooser));
    if(room->mixedletters != NULL)
        json_object_object_add(data, "mixedletters", json_object_new_string(room->mixedletters));
    json_object_object_add(data, "inGame", json_object_new_boolean(room->inGame));
    json_object_object_add(data, "address", json_object_new_string(room->address));
    json_object_object_add(data, "round", json_object_new_int(room->round));
    json_object_object_add(data, "players", players);
    for (int i = 0; i < room->numberOfPlayers; i++)
    {
        json_object *player = json_object_new_object();
        json_object_object_add(player, "username", json_object_new_string(room->players[i]->client.username));
        json_object_object_add(player, "avatar", json_object_new_int(room->players[i]->client.avatar));
        json_object_object_add(player, "score", json_object_new_int(room->players[i]->score));
        json_object_object_add(player, "status", json_object_new_string(getPlayerStatusString(room->players[i]->status)));
        json_object_array_add(players, player);
    }
    switch (room->language)
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
    room->maxPlayers = json_object_get_int(json_object_object_get(root, "maxNumberOfPlayers"));
    room->language = getLanguageFromString(json_object_get_string(json_object_object_get(root, "language")));
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
        //Players
        json_object *players = json_object_new_array();
        for (int j = 0; j < rooms[i].numberOfPlayers; j++)
        {
            json_object *player = json_object_new_object();
            json_object_object_add(player, "username", json_object_new_string(rooms[i].players[j]->client.username));
            json_object_object_add(player, "avatar", json_object_new_int(rooms[i].players[j]->client.avatar));
            json_object_object_add(player, "score", json_object_new_int(rooms[i].players[j]->score));
            json_object_object_add(player, "status", json_object_new_string(getPlayerStatusString(rooms[i].players[j]->status)));
            json_object_array_add(players, player);
        }
        json_object_object_add(room, "address", json_object_new_string(rooms[i].address));
        json_object_object_add(room, "round", json_object_new_int(rooms[i].round));
        json_object_object_add(room, "inGame", json_object_new_boolean(rooms[i].inGame));
        //language
        switch (rooms[i].language)
        {
        case ENGLISH:
            json_object_object_add(room, "language", json_object_new_string("en"));
            break;
        case ITALIAN:
            json_object_object_add(room, "language", json_object_new_string("it"));
            break;
        case SPANISH:
            json_object_object_add(room, "language", json_object_new_string("es"));
            break;
        case GERMAN:
            json_object_object_add(room, "language", json_object_new_string("de"));
            break;
        }
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