#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#include "handle_room.h"
#include "data/request.h"
#include "json/formatter.h"
#include "globals.h"

#define BUF_SIZE 1024
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
            }
        }

        if (request->type == "SERVER_MESSAGE")
        {
            fprintf(stderr, "Received server message: %s\n", request->data);
            pthread_mutex_unlock(&clients_mutex);
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
                        room->players[i]->score += 10;
                    }
                }
                pthread_mutex_unlock(&rooms_mutex);
            }
        }

        if(request->type == "NEW_CHOOSER")
        {
            fprintf(stderr, "Received server error: %s\n", request->data);

    
        }

        if (newRound)
        {
            json_object *root = json_object_new_object();
            json_object_object_add(root, "requestType", json_object_new_string("NEW_CHOOSER"));
            pthread_mutex_lock(&rooms_mutex);
            int chooserIndex = rand() % room->numberOfPlayers;
            json_object_object_add(root, "data", json_object_new_string(room->players[chooserIndex]->client.username));
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

    // Close the socket
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}