#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <multicast.h>

#include "handle_room.h"
#include "data/response.h"
#include "json/formatter.h"
#include "globals.h"

#define BUF_SIZE 1024

Room *room;
bool running = true;

void alarmHandler(int sig)
{
    fprintf(stderr, "Time is up!\n");
    pthread_mutex_lock(&rooms_mutex);
    for (int i = 0; i < num_rooms; i++)
    {
        if (rooms[i].inGame = 1)
        {
            rooms[i].round++;
            rooms[i].inGame = 0;
            for (int j = 0; j < rooms[i].numberOfPlayers; j++)
            {
                if (rooms[i].players[j]->status == CHOOSER && rooms[i].players[j]->status == SPECTATOR)
                {
                    rooms[i].players[j]->status = GUESSER;
                }
            }
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
}

void *reveal_letters(void *arg)
{
    Room *room = (Room *)arg;

    char *mixedletters = (char *)malloc(strlen(room->mixedletters) + 1);
    strcpy(mixedletters, room->mixedletters);
    room->revealedletters = (char *)malloc(strlen(room->word) + 1);
    room->revealedletters[0] = '\0';

    while (1)
    {
        sleep(15);

        // Lock the room
        pthread_mutex_lock(&rooms_mutex);

        // Check if the game is still in progress
        if (!room->inGame)
        {
            pthread_mutex_unlock(&rooms_mutex);
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
            room->round++;
        }

        // Unlock the room
        pthread_mutex_unlock(&rooms_mutex);
    }

    return NULL;
}

void close_room(Room *room)
{
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
    running = false;
}

void cb(struct mcpacket *packet)
{
    bool newGame = false;
    // printf("[%s]-[Len:%d]-[From:%s]\n", packet->data, packet->len, packet->src_addr);
    Response *response = parseResponse(packet->data);

    if (strcmp(response->type, "SERVER_NOTIFICATION") == 0)
    {
        const char *notification = json_object_get_string(response->data);
        fprintf(stderr, "Received server notify: %s\n", notification);

        json_object *root = response->data;
        const char *whatHappened = json_object_get_string(json_object_object_get(root, "whatHappened"));
        if (strcmp(whatHappened, "JOINED") == 0)
        {
            pthread_mutex_lock(&rooms_mutex);
            json_object *player = json_object_object_get(root, "player");
            const char *username = json_object_get_string(json_object_object_get(player, "username"));
            room->players[room->numberOfPlayers] = (Player *)malloc(sizeof(Player));
            pthread_mutex_unlock(&clients_mutex);
            Client *client;
            for (int i = 0; i < num_clients; i++)
            {
                if (strcmp(clients[i].username, username) == 0)
                {
                    client = &clients[i];
                }
            }
            room->players[room->numberOfPlayers]->client = client;
            pthread_mutex_unlock(&clients_mutex);
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
                    newGame = true;
                }
            }
            pthread_mutex_unlock(&rooms_mutex);
        }

        if (strcmp(whatHappened, "LEFT") == 0)
        {
            const char *username = json_object_get_string(json_object_object_get(root, "username"));
            pthread_mutex_lock(&rooms_mutex);
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
                close_room(room);
            }
            pthread_mutex_unlock(&rooms_mutex);
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
            pthread_mutex_lock(&rooms_mutex);
            room->round++;
            newGame = true;
            // The alarm is reset
            alarm(0);
            alarm(0.1);
            fprintf(stderr, "The alarm has been reset\n");
            for (int i = 0; i < room->numberOfPlayers; i++)
            {
                pthread_mutex_lock(&clients_mutex);
                if (strcmp(room->players[i]->client->username, username) == 0)
                {
                    room->players[i]->score += strlen(room->word);
                    fprintf(stderr, "The score of %s is now %d\n", username, room->players[i]->score);
                }
                pthread_mutex_unlock(&clients_mutex);
                pthread_mutex_unlock(&rooms_mutex);
            }
        }
    }

    if (strcmp(response->type, "WORD_CHOSEN") == 0)
    {
        fprintf(stderr, "Received server message: %s\n", json_object_get_string(response->data));
        pthread_mutex_unlock(&clients_mutex);
        json_object *root = response->data;
        const char *word = json_object_get_string(json_object_object_get(root, "word"));
        const char *mixedletters = json_object_get_string(json_object_object_get(root, "mixedletters"));
        pthread_mutex_lock(&rooms_mutex);
        room->word = word;
        room->mixedletters = mixedletters;
        room->inGame = 1;
        pthread_mutex_unlock(&rooms_mutex);
        pthread_t reveal_thread;
        if (pthread_create(&reveal_thread, NULL, reveal_letters, room) != 0)
        {
            perror("pthread_create() error");
            close_room(room);
        }
        alarm(15 * strlen(mixedletters));
    }

    if (newGame)
    {
        sleep(5);
        json_object *root = json_object_new_object();
        json_object_object_add(root, "responseType", json_object_new_string("NEW_CHOOSER"));
        pthread_mutex_lock(&rooms_mutex);
        int chooserIndex = rand() % room->numberOfPlayers;
        room->round = 0;
        pthread_mutex_unlock(&clients_mutex);
        json_object_object_add(root, "data", json_object_new_string(room->players[chooserIndex]->client->username));
        pthread_mutex_unlock(&clients_mutex);
        room->players[chooserIndex]->status = CHOOSER;
        pthread_mutex_unlock(&rooms_mutex);
        const char *json = json_object_to_json_string(root);

        struct mcsender *sc = mc_sender_init(NULL, room->address, SERVER_PORT);
        mc_sender_send(sc, json, strlen(json) + 1);
        mc_sender_uinit(sc);

        newGame = false;
    }
}

void *handle_room(void *arg)
{

    room = (Room *)arg;

    fprintf(stderr, "Ho creato il Thread per la stanza %s\n", room->name);

    signal(SIGALRM, alarmHandler);
    struct mcreceiver *rc = mc_receiver_init(NULL, room->address, SERVER_PORT, &cb);

    while (running)
    {
        sleep(1);
    }
    mc_receiver_uinit(rc);
    return 0;
}
