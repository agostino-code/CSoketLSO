#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#include "handle_room.h"
#include "handle_player.h"
#include "globals.h"

void broadcast_to_room(const Room *room, const char *message)
{
    for (int i = 0; i < room->numberOfPlayers; i++)
    {
        send(room->players[i].client.socket, message, strlen(message), 0);
    }
}

void *handle_room(void *arg)
{
    Room *room = (Room *)arg;

    // Create a socket for the room
    const int room_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (room_socket == -1)
    {
        printf("Could not create socket for room %s\n", room->name);
        return NULL;
    }

    // Bind the socket to the room port

    struct sockaddr_in room_address;
    room_address.sin_family = AF_INET;
    room_address.sin_addr.s_addr = INADDR_ANY;
    room_address.sin_port = htons(room->address.sin_port);

    if (bind(room_socket, (struct sockaddr *)&room_address, sizeof(room_address)) < 0)
    {
        printf("Could not bind socket to room %s\n", room->name);
        return NULL;
    }

    pthread_mutex_lock(&rooms_mutex);
    room->inGame = false;
    room->address = room_address;
    room->socket = room_socket;
    room->thread = pthread_self();
    pthread_mutex_unlock(&rooms_mutex);
    // Listen for connections

    if (listen(room_socket, 5) < 0)
    {
        printf("Could not listen for connections on room %s\n", room->name);
        return NULL;
    }

    // Accept connections from clients
    while (1)
    {
        struct sockaddr_in client_addr;
        int client_address_length = sizeof(client_addr);

        int client_socket = accept(room_socket, (struct sockaddr *)&client_addr,
                                   (socklen_t *)&client_address_length);
        if (client_socket < 0)
        {
            printf("Could not accept client connection on room %s\n", room->name);
            continue;
        }
        {
            printf("Client connected to room %s\n", room->name);

            // Add the client to the players array
            pthread_mutex_lock(&rooms_mutex);
            if (room->numberOfPlayers < room->maxPlayers)
            {
                const Client *client = find_client_by_socket(client_socket);
                if (strcmp(client->username, "Anonymous") != 0)
                {
                    room->players[room->numberOfPlayers].client = *client;
                    room->players[room->numberOfPlayers].score = 0;
                    if (room->inGame)
                    {
                        room->players[room->numberOfPlayers].status = SPECTATOR;
                    }
                    else
                    {
                        room->players[room->numberOfPlayers].status = GUESSER;
                    }
                    room->numberOfPlayers++;
                    printf("Client connected to room %s\n", room->name);
                }
                else
                {
                    printf("Client not logged in\n");
                }
            }
            else
            {
                printf("Room is full\n");
            }
            pthread_mutex_unlock(&rooms_mutex);

            // Broadcast to all clients in the room
            char message[1024];
            sprintf(message, "Client %s:%d connected to room %s\n", inet_ntoa(client_addr.sin_addr),
                    ntohs(client_addr.sin_port), room->name);
            broadcast_to_room(room, message);

            // Create a thread for the client
            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_player, &client_socket) != 0)
            {
                printf("Could not create thread for client in room %s\n", room->name);
            }

            // Detach the thread
            pthread_detach(thread);
        }

        if (room->numberOfPlayers == 2)
        {
            pthread_mutex_lock(&rooms_mutex);
            room->inGame = true;
            pthread_mutex_unlock(&rooms_mutex);
            // Send a message to all clients in the room
            char message[1024];
            sprintf(message, "Room %s is full. The game is starting\n", room->name);
            broadcast_to_room(room, message);

            // Start the game
            // Select an chooser
            int chooser = rand() % 2;
            room->players[chooser].status = CHOOSER;
            // Send the game information to all clients in the room
            const char *json = roomToJson(room);
            broadcast_to_room(room, json);

            // Wait the word sended by the chooser
            char word[1024];
            recv(room->players[chooser].client.socket, word, 1024, 0);
            printf("Word received: %s\n", word);
            // Send the word to the other player
            send(room->players[1 - chooser].client.socket, word, strlen(word), 0);
            // Start the game loop
        }
    }
}