#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "handle_room.h"
#include "data/request.h"
#include "json/formatter.h"
#include "globals.h"

#define BUF_SIZE 1024

void alarmHandler(int sig)
{
    fprintf(stderr, "Time is up!\n");
    pthread_mutex_lock(&rooms_mutex);
    for (int i = 0; i < num_rooms; i++)
    {
        if (rooms[i].inGame)
        {
            rooms[i].round++;
            rooms[i].inGame = 0;
            for (int j = 0; j < rooms[i].numberOfPlayers; j++)
            {
                if (rooms[i].players[j]->status == CHOOSER)
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

    char *mixedletters = room->mixedletters;

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
        if (strlen(room->mixedletters) > 0)
        {
            strncat(room->revealedletters, room->mixedletters, 1);
            memmove(mixedletters, room->mixedletters + 1, strlen(room->mixedletters));
        }

        // Unlock the room
        pthread_mutex_unlock(&rooms_mutex);
    }

    return NULL;
}

void *handle_room(void *arg)
{
    fprintf(stderr, "handle_room\n");

    Room *room = (Room *)arg;
    /* Create a datagram socket on which to receive. */
    int sock;
    struct sockaddr_in addr;
    struct ip_mreq join_addr;
    char buf[1024];
    int str_len, addr_len;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);
    addr_len = sizeof(addr);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        error_handling("bind() error");

    join_addr.imr_multiaddr.s_addr = inet_addr(room->address);
    join_addr.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_addr, sizeof(join_addr)) == -1)
        error_handling("setsockopt() error");

    bool newRound = false;
    signal(SIGALRM, alarmHandler);
    while (1)
    {
        str_len = recvfrom(sock, buf, BUF_SIZE - 1, 0, (struct sockaddr *)&addr, &addr_len);
        if (str_len < 0)
        {
            perror("recvfrom() error");
            break;
        }
        if (str_len > 0)
            buf[str_len] = '\0';

        fprintf(stderr, "Received: %s\n", buf);

        // Send the received message to the client
        if (sendto(sock, buf, str_len, 0, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("sendto() error");
        }

        Request *request = parseRequest(buf);

        if (request->type == "SERVER_NOTIFICATION")
        {
            fprintf(stderr, "Received server notify: %s\n", request->data);

            json_object *root = request->data;
            const char *whatHappened = json_object_get_string(json_object_object_get(root, "whatHappened"));
            if (strcmp(whatHappened, "JOINED") == 0)
            {
                pthread_mutex_lock(&rooms_mutex);
                fprintf(stderr, "Received server notify: %s\n", request->data);
                json_object *player = json_object_object_get(root, "user");
                const char *username = json_object_get_string(json_object_object_get(player, "username"));
                const char *avatar = json_object_get_string(json_object_object_get(player, "avatar"));
                room->players[room->numberOfPlayers] = (Player *)malloc(sizeof(Player));
                room->players[room->numberOfPlayers]->client.username = username;
                room->players[room->numberOfPlayers]->client.avatar = atoi(avatar);
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
                        newRound = true;
                    }
                }
                pthread_mutex_unlock(&rooms_mutex);
            }

            if (strcmp(whatHappened, "LEFT") == 0)
            {
                fprintf(stderr, "Received server notify: %s\n", request->data);
                json_object *player = json_object_object_get(root, "user");
                const char *username = json_object_get_string(json_object_object_get(player, "username"));
                pthread_mutex_lock(&rooms_mutex);
                for (int i = 0; i < room->numberOfPlayers; i++)
                {
                    if (strcmp(room->players[i]->client.username, username) == 0)
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
                    break;
                }
                pthread_mutex_unlock(&rooms_mutex);
                continue;
            }
        }

        if (request->type == "SERVER_MESSAGE")
        {
            fprintf(stderr, "Received server message: %s\n", request->data);
            json_object *root = request->data;
            const char *message = json_object_get_string(json_object_object_get(root, "message"));
            const char *username = json_object_get_string(json_object_object_get(root, "username"));
            bool isGuessed = json_object_get_boolean(json_object_object_get(root, "isGuessed"));
            if (isGuessed)
            {
                pthread_mutex_lock(&rooms_mutex);
                room->round++;
                newRound = true;
                for (int i = 0; i < room->numberOfPlayers; i++)
                {
                    if (strcmp(room->players[i]->client.username, username) == 0)
                    {
                        // TODO: Aggiungere punteggio pari alla lunghezza della parola
                        room->players[i]->score += strlen(room->word);
                        // TODO: Se il tempo di gioco è finito aumenta lo score dello CHOOSER di 15 che uguale al max - la lunghezza della parola, fai il max tra 1 e 15 - lunghezza parola
                    }
                }
                pthread_mutex_unlock(&rooms_mutex);
            }
        }

        if (request->type == "WORD_CHOSEN")
        {
            fprintf(stderr, "Received server message: %s\n", request->data);
            pthread_mutex_unlock(&clients_mutex);
            json_object *root = request->data;
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
                return NULL;
            }
            alarm(15*strlen(mixedletters));
            continue;
        }

        if (newRound)
        {
            json_object *root = json_object_new_object();
            json_object_object_add(root, "requestType", json_object_new_string("NEW_CHOOSER"));
            pthread_mutex_lock(&rooms_mutex);
            int chooserIndex = rand() % room->numberOfPlayers;

            json_object_object_add(root, "data", json_object_new_string(room->players[chooserIndex]->client.username));
            room->players[chooserIndex]->status = CHOOSER;
            pthread_mutex_unlock(&rooms_mutex);
            const char *json = json_object_to_json_string(root);

            sendto(sock, json, strlen(json), 0, (struct sockaddr *)&addr, sizeof(addr));

            newRound = false;
        }
    }

    // elimina room dall'array di rooms
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
    free(room);

    // Close the socket
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}
