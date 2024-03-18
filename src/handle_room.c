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

/*
    - If the game is finished, check if the winner is the chooser or a guesser
    - If the winner is the chooser, the score is 15 - the length of the word
    - If the winner is a guesser, the score is the length of the word - the length of the revealed letters
*/
void finish_game(const char *username)
{
    if (room->inGame == true)
    {

        fprintf(stderr, "The game is finished\n");
        pthread_mutex_lock(&room->mutex);
        room->inGame = false;
        pthread_mutex_unlock(&room->mutex);
        bool player_ingame = false;

        for (int i = 0; i < room->numberOfPlayers; i++)
        {
            if (strcmp(room->players[i]->client->username, username) == 0)
            {
                player_ingame = true;
            }
        }

        if (player_ingame == true)
        {
            if (strcmp(room->chooser, username) == 0)
            {
                fprintf(stderr, "Time is up!\n");

                for (int i = 0; i < room->numberOfPlayers; i++)
                {
                    pthread_mutex_lock(&room->mutex);
                    room->players[i]->status = GUESSER;

                    if (strcmp(room->players[i]->client->username, username) == 0)
                    {
                        if (15 - strlen(room->word) > 0)
                            room->players[i]->score += 15 - strlen(room->word);
                        else
                            room->players[i]->score += 0;
                        fprintf(stderr, "The score of %s is now %d\n", username, room->players[i]->score);
                    }
                    pthread_mutex_unlock(&room->mutex);
                }
            }
            else
            {
                for (int i = 0; i < room->numberOfPlayers; i++)
                {
                    pthread_mutex_lock(&room->mutex);
                    room->players[i]->status = GUESSER;
                    pthread_mutex_unlock(&room->mutex);

                    if (strcmp(room->players[i]->client->username, username) == 0)
                    {
                        if (strlen(room->revealedletters) != 0 && room->revealedletters != NULL)
                        {
                            int word_length = strlen(room->word);
                            int revealed_length = strlen(room->revealedletters);
                            int word_with_revealed_length = 0;

                            for (int i = 0; i < word_length; i++)
                            {
                                for (int j = 0; j < revealed_length; j++)
                                {
                                    if (room->word[i] == room->revealedletters[j])
                                    {
                                        word_with_revealed_length++;
                                    }
                                }
                            }
                            pthread_mutex_lock(&room->mutex);
                            room->players[i]->score += word_length - word_with_revealed_length;
                            pthread_mutex_unlock(&room->mutex);
                        }
                        else
                        {
                            pthread_mutex_lock(&room->mutex);
                            room->players[i]->score += strlen(room->word);
                            pthread_mutex_unlock(&room->mutex);
                        }

                        fprintf(stderr, "The score of %s is now %d\n", username, room->players[i]->score);
                    }
                }
            }
        }

        pthread_mutex_lock(&room->mutex);
        room->chooser = NULL;
        room->word = NULL;
        room->mixedletters = NULL;
        room->revealedletters = NULL;
        pthread_mutex_unlock(&room->mutex);

        if (room->numberOfPlayers > 1)
            start_game();
    }
}

/*
    - The function is called when the game is started
    - The function reveals a letter every 15 seconds
*/

void *reveal_letters(void *arg)
{
    char *username = (char *)arg;

    pthread_mutex_lock(&room->mutex);
    char *mixedletters = (char *)malloc(strlen(room->mixedletters) + 1);
    strcpy(mixedletters, room->mixedletters);
    room->revealedletters = (char *)malloc(strlen(room->word) + 1);
    room->revealedletters[0] = '\0';
    pthread_mutex_unlock(&room->mutex);

    int index = 0;
    while (room->inGame == true)
    {
        sleep(15);
        // Take a letter from mixedLetters and insert it into revealedLetters
        if (room->inGame == true)
        {
            if (strlen(mixedletters) > 0)
            {
                pthread_mutex_lock(&room->mutex);
                char temp = mixedletters[index];
                strcat(room->revealedletters, &temp);
                index++;
                pthread_mutex_unlock(&room->mutex);

                fprintf(stderr, "Revealed letters: %s\n", room->revealedletters);
            }

            if (index == strlen(mixedletters))
            {
                free(mixedletters);
                fprintf(stderr, "No more letters to reveal\n");
                finish_game(username);
                break;
            }
        }
    }
    free(mixedletters);
    return NULL;
}

/*
    - The function is called when the game is finished
    - The function closes the room if there are no players
*/

void close_room()
{
    pthread_mutex_lock(&rooms_mutex);
    for (int i = 0; i < num_rooms; i++)
    {
        if (&rooms[i] == room)
        {
            for (int j = i; j < num_rooms - 1; j++)
            {
                rooms[j] = rooms[j + 1];
            }
            num_rooms--;
            //free(room);
            break;
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
    running = false;
}

/*
    - The function is called when the game is started
    - The function chooses a random player to be the chooser
    - The function sends a notification to all players
*/

void start_game()
{
    if (room->inGame == false)
        if (room->numberOfPlayers > 1)
        {
            fprintf(stderr, "New game\n");
            sleep(15);
            if (room->numberOfPlayers > 1)
            {
                json_object *root = json_object_new_object();
                json_object_object_add(root, "responseType", json_object_new_string("NEW_CHOOSER"));
                int chooserIndex = rand() % room->numberOfPlayers;
                pthread_mutex_lock(&room->mutex);
                room->chooser = room->players[chooserIndex]->client->username;
                pthread_mutex_unlock(&room->mutex);

                bool player_ingame = false;
                while (player_ingame == false)
                {
                    for (int i = 0; i < room->numberOfPlayers; i++)
                    {
                        if (strcmp(room->players[i]->client->username, room->chooser) == 0)
                        {
                            player_ingame = true;
                        }
                    }
                    json_object_object_add(root, "data", json_object_new_string(room->players[chooserIndex]->client->username));
                    pthread_mutex_lock(&room->mutex);
                    room->players[chooserIndex]->status = CHOOSER;
                    pthread_mutex_unlock(&room->mutex);
                }

                const char *json = json_object_to_json_string(root);
                fprintf(stderr, "Sending to multicast: %s\n", json);
                struct mcsender *sc = mc_sender_init(NULL, room->address, SERVER_PORT);
                mc_sender_send(sc, json, strlen(json) + 1);
                mc_sender_uinit(sc);
                alarm(35);
            }
        }
}

/*
    - The function is called when a packet is received
    - The function parses the response and checks the type
    - The function handles the SERVER_NOTIFICATION type
    - The function handles the SERVER_MESSAGE type
    - The function handles the WORD_CHOSEN type
*/

void cb(struct mcpacket *packet)
{
    // printf("[%s]-[Len:%d]-[From:%s]\n", packet->data, packet->len, packet->src_addr);
    if (packet->len == 0)
        return;

    Response *response = parseResponse(packet->data);

    if (response == NULL)
        return;

    if (response->type == NULL)
        return;

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
            pthread_mutex_lock(&room->mutex);
            room->players[room->numberOfPlayers] = (Player *)malloc(sizeof(Player));
            pthread_mutex_unlock(&room->mutex);
            Client *client;
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < num_clients; i++)
            {
                if (strcmp(clients[i].username, username) == 0)
                {
                    client = &clients[i];
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            room->players[room->numberOfPlayers]->client = client;
            room->players[room->numberOfPlayers]->score = 0;
            pthread_mutex_lock(&room->mutex);
            room->numberOfPlayers++;
            pthread_mutex_unlock(&room->mutex);
            if (room->inGame == true)
            {
                pthread_mutex_lock(&room->mutex);
                room->players[room->numberOfPlayers - 1]->status = SPECTATOR;
                pthread_mutex_unlock(&room->mutex);
            }
            else
            {
                pthread_mutex_lock(&room->mutex);
                room->players[room->numberOfPlayers - 1]->status = GUESSER;
                pthread_mutex_unlock(&room->mutex);

                if (room->numberOfPlayers - 1 == 1)
                {
                    start_game();
                }
            }
        }

        if (strcmp(whatHappened, "LEFT") == 0)
        {
            json_object *player = json_object_object_get(root, "player");
            const char *username = json_object_get_string(json_object_object_get(player, "username"));
            if (room->chooser != NULL)
            {
                if (strcmp(username, room->chooser) == 0)
                {
                    if (room->inGame == true)
                        if (room->numberOfPlayers > 0)
                            start_game();
                }
            }

            for (int i = 0; i < room->numberOfPlayers; i++)
            {
                if (strcmp(room->players[i]->client->username, username) == 0)
                {
                    pthread_mutex_lock(&room->mutex);
                    room->players[i]=NULL;

                    for (int j = i; j < room->numberOfPlayers - 1; j++)
                    {
                        room->players[j] = room->players[j + 1];
                    }
                    room->numberOfPlayers--;
                    pthread_mutex_unlock(&room->mutex);
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
        alarm(0);
        fprintf(stderr, "Received server message: %s\n", json_object_get_string(response->data));
        json_object *root = response->data;
        const char *word = json_object_get_string(json_object_object_get(root, "word"));
        const char *mixedletters = json_object_get_string(json_object_object_get(root, "mixedletters"));
        pthread_mutex_lock(&room->mutex);
        room->word = word;
        room->mixedletters = mixedletters;
        room->inGame = true;
        pthread_mutex_unlock(&room->mutex);
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
    room->chooser = NULL;
    room->word = NULL;
    room->mixedletters = NULL;
    room->revealedletters = NULL;
    room->inGame = false;
    room->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    fprintf(stderr, "Ho creato il Thread per la stanza %s\n", room->name);
    struct mcreceiver *rc = mc_receiver_init(NULL, room->address, SERVER_PORT, &cb);
    signal(SIGALRM, alarm_handler);
    while (room->numberOfPlayers > 0)
    {
        sleep(1);
    }
    close_room();
    mc_receiver_uinit(rc);
    return 0;
}

/*
    - If the player not chosen a word in 30 seconds, the game chooses another player
*/

void alarm_handler(int signum)
{
    if (room->inGame == true)
        finish_game(room->chooser);
    else if (room->inGame == false && room->numberOfPlayers > 0)
        start_game();
}
