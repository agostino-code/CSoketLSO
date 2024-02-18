#ifndef FORMATTER_H
#define FORMATTER_H

#include <yyjson.h>
#include "../data/user.h"
#include "../data/room.h"
#include "../data/player.h"

const char *userToJson(const User *userObj)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_val *user = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "responseType", "SUCCESS");
    yyjson_mut_obj_add(root, yyjson_mut_strn(doc, "data", 4), user);
    yyjson_mut_obj_add_str(doc, user, "email", userObj->email);
    yyjson_mut_obj_add_str(doc, user, "username", userObj->username);
    yyjson_mut_obj_add_str(doc, user, "password", userObj->password);
    yyjson_mut_obj_add_int(doc, user, "avatar", userObj->avatar);

    const char *json = yyjson_mut_write(doc, 0, NULL);
    //    if (json) {
    //        printf("json: %s\n", json);
    //        free((void *)json);
    //    }

    printf("Response sent: %s\n", json);

    yyjson_mut_doc_free(doc);
    return json;
}

// userParse
User *userParse(const char *string)
{
    yyjson_doc *doc = yyjson_read(string, strlen(string), 0);
    if (!doc)
    {
        return NULL;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!root)
    {
        yyjson_doc_free(doc);
        return NULL;
    }
    User *user = (User *)malloc(sizeof(User));
    user->email = (char *)yyjson_get_str(yyjson_obj_get(root, "email"));
    user->username = (char *)yyjson_get_str(yyjson_obj_get(root, "username"));
    user->password = (char *)yyjson_get_str(yyjson_obj_get(root, "password"));
    user->avatar = yyjson_get_int(yyjson_obj_get(root, "avatar"));
    yyjson_doc_free(doc);
    return user;
}

// Send all room information to client
const char *roomToJson(const Room *roomObj)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_val *room = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "responseType", "SUCCESS");
    yyjson_mut_obj_add(root, yyjson_mut_strn(doc, "data", 4), room);
    yyjson_mut_obj_add_str(doc, room, "name", roomObj->name);
    yyjson_mut_obj_add_int(doc, room, "numberOfPlayers", roomObj->numberOfPlayers);
    yyjson_mut_obj_add_int(doc, room, "maxNumberOfPlayers", roomObj->maxPlayers);
    yyjson_mut_obj_add_int(doc, room, "port", roomObj->address.sin_port);
    // Round
    yyjson_mut_obj_add_int(doc, room, "round", roomObj->round);
    // Players
    yyjson_mut_val *players = yyjson_mut_arr(doc);
    yyjson_mut_obj_add(room, yyjson_mut_strn(doc, "players", 7), players);
    for (int i = 0; i < roomObj->numberOfPlayers; i++)
    {
        yyjson_mut_val *player = yyjson_mut_obj(doc);
        yyjson_mut_arr_append(players, player);
        yyjson_mut_obj_add_str(doc, player, "username", roomObj->players[i].client.username);
        // avatar
        yyjson_mut_obj_add_int(doc, player, "avatar", roomObj->players[i].client.avatar);
        yyjson_mut_obj_add_int(doc, player, "score", roomObj->players[i].score);
        if (roomObj->numberOfPlayers > 1)
        {
            switch (roomObj->players[i].status)
            {
            case SPECTATOR:
                yyjson_mut_obj_add_str(doc, player, "status", "spectator");
                break;
            case GUESSER:
                yyjson_mut_obj_add_str(doc, player, "status", "guesser");
                break;
            case CHOOSER:
                yyjson_mut_obj_add_str(doc, player, "status", "chooser");
                break;
            }
        }
    }
    switch (roomObj->language)
    {
    case ENGLISH:
        yyjson_mut_obj_add_str(doc, room, "language", "en");
        break;
    case ITALIAN:
        yyjson_mut_obj_add_str(doc, room, "language", "it");
        break;
    case SPANISH:
        yyjson_mut_obj_add_str(doc, room, "language", "es");
        break;
    case GERMAN:
        yyjson_mut_obj_add_str(doc, room, "language", "de");
        break;
    }

    const char *json = yyjson_mut_write(doc, 0, NULL);
    //    if (json) {
    //        printf("json: %s\n", json);
    //        free((void *)json);
    //    }

    printf("Response sent: %s\n", json);

    yyjson_mut_doc_free(doc);
    return json;
}

// roomParse
Room *roomParse(const char *string)
{
    yyjson_doc *doc = yyjson_read(string, strlen(string), 0);
    if (!doc)
    {
        return NULL;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!root)
    {
        yyjson_doc_free(doc);
        return NULL;
    }
    Room *room = (Room *)malloc(sizeof(Room));
    room->name = (char *)yyjson_get_str(yyjson_obj_get(root, "name"));
    room->numberOfPlayers = yyjson_get_int(yyjson_obj_get(root, "numberOfPlayers"));
    room->maxPlayers = yyjson_get_int(yyjson_obj_get(root, "maxNumberOfPlayers"));
    room->address.sin_port = yyjson_get_int(yyjson_obj_get(root, "port"));
    room->round = yyjson_get_int(yyjson_obj_get(root, "round"));
    yyjson_val *players = yyjson_obj_get(root, "players");
    for (int i = 0; i < room->numberOfPlayers; i++)
    {
        yyjson_val *player = yyjson_arr_get(players, i);
        room->players[i].client.username = (char *)yyjson_get_str(yyjson_obj_get(player, "username"));
        room->players[i].client.avatar = yyjson_get_int(yyjson_obj_get(player, "avatar"));
        room->players[i].score = yyjson_get_int(yyjson_obj_get(player, "score"));
        if (room->numberOfPlayers > 1)
        {

            room->players[i].status = getPlayerStatusFromString(yyjson_get_str(yyjson_obj_get(player, "status"))); // Assign the correct player status based on the JSON value
        }
    }
    room->language = getLanguageFromString(yyjson_get_str(yyjson_obj_get(root, "language")));
    return room;
}

// Function for creating a JSON error message
const char *createJsonErrorMessage(const char *errorMessage)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "responseType", "ERROR");
    yyjson_mut_obj_add_str(doc, root, "data", errorMessage);

    const char *json = yyjson_mut_write(doc, 0, NULL);
    //    if (json) {
    //        printf("json: %s\n", json);
    //        free((void *)json);
    //    }
    printf("Response sent: %s\n", json);

    yyjson_mut_doc_free(doc);
    return json;
}

// Function for creating a JSON success message
const char *createJsonSuccessMessage(const char *successMessage)
{
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "responseType", "SUCCESS");
    yyjson_mut_obj_add_str(doc, root, "data", successMessage);

    const char *json = yyjson_mut_write(doc, 0, NULL);
    //    if (json) {
    //        printf("json: %s\n", json);
    //        free((void *)json);
    //    }
    printf("Response sent: %s\n", json);

    yyjson_mut_doc_free(doc);
    return json;
}

const char *createJsonListOfRooms()
{
    pthread_mutex_lock(&rooms_mutex);
    // Create a JSON string from the rooms array
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_val *data = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "responseType", "SUCCESS");
    yyjson_mut_obj_add(root, yyjson_mut_strn(doc, "data", 4), data);

    for (int i = 0; i < num_rooms; i++)
    {
        yyjson_mut_val *room = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, room, "name", rooms[i].name);
        yyjson_mut_obj_add_int(doc, room, "numberOfPlayers", rooms[i].numberOfPlayers);
        yyjson_mut_obj_add_int(doc, room, "maxNumberOfPlayers", rooms[i].maxPlayers);
        yyjson_mut_obj_add_int(doc, room, "port", rooms[i].address.sin_port);
        switch (rooms[i].language)
        {
        case ENGLISH:
            yyjson_mut_obj_add_str(doc, room, "language", "en");
            break;
        case ITALIAN:
            yyjson_mut_obj_add_str(doc, room, "language", "it");
            break;
        case SPANISH:
            yyjson_mut_obj_add_str(doc, room, "language", "es");
            break;
        case GERMAN:
            yyjson_mut_obj_add_str(doc, room, "language", "de");
            break;
        }
        yyjson_mut_arr_append(data, room);
    }

    const char *json = yyjson_mut_write(doc, 0, NULL);
    printf("Response sent: %s\n", json);

    yyjson_mut_doc_free(doc);

    pthread_mutex_unlock(&rooms_mutex);
    return json;
}

// string to REQUEST
Request *parseRequest(const char *string)
{
    yyjson_doc *doc = yyjson_read(string, strlen(string), 0);
    if (!doc)
    {
        return NULL;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!root)
    {
        yyjson_doc_free(doc);
        return NULL;
    }
    Request *request = (Request *)malloc(sizeof(Request));
    request->type = (char *)yyjson_get_str(yyjson_obj_get(root, "requestType"));
    request->data = (char *)yyjson_get_str(yyjson_obj_get(root, "data"));
    yyjson_doc_free(doc);
    return request;
}

#endif
