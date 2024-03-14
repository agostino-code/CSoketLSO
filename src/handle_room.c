#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <multicast.h>
#include <stdio.h>

#include "handle_room.h"
#include "data/response.h"
#include "json/formatter.h"
#include "globals.h"

#define BUF_SIZE 1024

Room *room;
bool running = true;

void finish_game(const char *username)
{
    if (room->inGame == 1)
    {
        room->inGame = 0;
        if (strcmp(room->chooser, username) == 0)
        {
            fprintf(stderr, "Time is up!\n");

            for (int i = 0; i < room->numberOfPlayers; i++)
            {
                if (room->players[i]->status == SPECTATOR)
                {
                    room->players[i]->status = GUESSER;
                }

                if (strcmp(room->players[i]->client->username, username) == 0)
                {
                    room->players[i]->score += max(0,15 - strlen(room->word));
                    room->players[i]->status = GUESSER;
                    fprintf(stderr, "The score of %s is now %d\n", username, room->players[i]->score);
                }
            }
        }
        else
        {
            for (int i = 0; i < room->numberOfPlayers; i++)
            {
                if (room->players[i]->status == SPECTATOR)
                {
                    room->players[i]->status = GUESSER;
                }

                if (strcmp(room->players[i]->client->username, username) == 0)
                {
                    room->players[i]->score += strlen(room->word);
                    fprintf(stderr, "The score of %s is now %d\n", username, room->players[i]->score);
                }
            }
        }
        room->word = NULL;
        room->mixedletters = NULL;
        room->revealedletters = NULL;
        start_game();
    }
}

void *reveal_letters(void *arg)
{
    char *username = (char *)arg;

    char *mixedletters = (char *)malloc(strlen(room->mixedletters) + 1);
    strcpy(mixedletters, room->mixedletters);
    room->revealedletters = (char *)malloc(strlen(room->word) + 1);
    room->revealedletters[0] = '\0';

    while (1)
    {
        sleep(15);

        // Check if the game is still in progress
        if (!room->inGame)
        {
            free(mixedletters);
            break;
        }

        // Take a letter from mixedLetters and insert it into revealedLetters
        if (strlen(mixedletters) > 0)
        {
            room->revealedletters[strlen(room->revealedletters)] = mixedletters[0];
            room->revealedletters[strlen(room->revealedletters) + 1] = '\0';
            for (int i = 0; i < strlen(mixedletters); i++)
            {
                mixedletters[i] = mixedletters[i + 1];
            }
        }
        else
        {
            room->inGame = 0;
            finish_game(username);
        }
    }

    return NULL;
}

void close_room()
{
    pthread_mutex_lock(&rooms_mutex);
    for (int i = 0; i < num_rooms; i++)
    {
        if (&rooms[i] == room)
        {
            free(room);
            for (int j = i; j < num_rooms - 1; j++)
            {
                rooms[j] = rooms[j + 1];
            }
            num_rooms--;
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
    running = false;
}

void start_game()
{
    fprintf(stderr, "New game\n");
    sleep(5);
    json_object *root = json_object_new_object();
    json_object_object_add(root, "responseType", json_object_new_string("NEW_CHOOSER"));
    int chooserIndex = rand() % room->numberOfPlayers;
    room->chooser = room->players[chooserIndex]->client->username;
    json_object_object_add(root, "data", json_object_new_string(room->players[chooserIndex]->client->username));
    room->players[chooserIndex]->status = CHOOSER;

    const char *json = json_object_to_json_string(root);
    fprintf(stderr, "Sending to multicast: %s\n", json);
    struct mcsender *sc = mc_sender_init(NULL, room->address, SERVER_PORT);
    mc_sender_send(sc, json, strlen(json) + 1);
    mc_sender_uinit(sc);
}

void cb(struct mcpacket *packet)
{
    // printf("[%s]-[Len:%d]-[From:%s]\n", packet->data, packet->len, packet->src_addr);ù
    Response *response = parseResponse(packet->data);

    if (strcmp(response->type, "SERVER_NOTIFICATION") == 0)
    {
        const char *notification = json_object_get_string(response->data);
        fprintf(stderr, "Received server notify: %s\n", notification);

        json_object *root = response->data;
        const char *whatHappened = json_object_get_string(json_object_object_get(root, "whatHappened"));
        if (strcmp(whatHappened, "JOINED") == 0)
        {
            json_object *player = json_object_object_get(root, "player");
            const char *username = json_object_get_string(json_object_object_get(player, "username"));
            room->players[room->numberOfPlayers] = (Player *)malloc(sizeof(Player));
            Client *client;
            for (int i = 0; i < num_clients; i++)
            {
                if (strcmp(clients[i].username, username) == 0)
                {
                    client = &clients[i];
                }
            }
            room->players[room->numberOfPlayers]->client = client;
            room->players[room->numberOfPlayers]->score = 0;
            room->numberOfPlayers++;
            if (room->inGame == true)
            {
                room->players[room->numberOfPlayers - 1]->status = SPECTATOR;
            }
            else
            {
                room->players[room->numberOfPlayers - 1]->status = GUESSER;
                if (room->numberOfPlayers > 1)
                {
                    room->inGame = 1;
                    start_game();
                }
            }
        }

        if (strcmp(whatHappened, "LEFT") == 0)
        {
            const char *username = json_object_get_string(json_object_object_get(root, "username"));
            for (int i = 0; i < room->numberOfPlayers; i++)
            {
                if (strcmp(room->players[i]->client->username, username) == 0)
                {
                    free(room->players[i]);
                    for (int j = i; j < room->numberOfPlayers - 1; j++)
                    {
                        room->players[j] = room->players[j + 1];
                    }
                    room->numberOfPlayers--;
                }
            }
            if (room->numberOfPlayers == 0)
            {
                close_room();
            }
        }
    }

    if (strcmp(response->type, "SERVER_MESSAGE") == 0)
    {
        fprintf(stderr, "Received server message: %s\n", json_object_get_string(response->data));
        json_object *root = response->data;
        const char *message = json_object_get_string(json_object_object_get(root, "message"));
        const char *username = json_object_get_string(json_object_object_get(root, "username"));
        bool isGuessed = json_object_get_boolean(json_object_object_get(root, "isGuessed"));
        if (isGuessed)
        {

            fprintf(stderr, "The word has been guessed by %s\n", username);

            // The alarm is reset
            finish_game(username);
        }
    }

    if (strcmp(response->type, "WORD_CHOSEN") == 0)
    {
        fprintf(stderr, "Received server message: %s\n", json_object_get_string(response->data));
        json_object *root = response->data;
        const char *word = json_object_get_string(json_object_object_get(root, "word"));
        const char *mixedletters = json_object_get_string(json_object_object_get(root, "mixedletters"));
        room->word = word;
        room->mixedletters = mixedletters;
        room->inGame = 1;
        pthread_t reveal_thread;
        if (pthread_create(&reveal_thread, NULL, reveal_letters, (void *)room->chooser) != 0)
        {
            perror("pthread_create() error");
            close_room();
        }
    }
}

void *handle_room(void *arg)
{
    room = (Room *)arg;

    fprintf(stderr, "Ho creato il Thread per la stanza %s\n", room->name);
    struct mcreceiver *rc = mc_receiver_init(NULL, room->address, SERVER_PORT, &cb);
    while (running)
    {
        sleep(1);
    }
    mc_receiver_uinit(rc);
    return 0;
}
