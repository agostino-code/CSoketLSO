#include <unistd.h>
#include <arpa/inet.h>
#include "data/request.h"
#include "globals.h"
#include "json/formatter.h"
#include "handle_client.h"
#include "handle_room.h"

void *handle_client(void *arg)
{
    int client_socket = *(int *)arg;
    char buffer[1024];
    int n;

    /*
        The `read` function is called to read data from the client socket.
        If the return value is less than 0, this means an error occurred while reading from the socket.
        If the return value is 0, this means the client has disconnected from the server.
    */

    while (1)
    {
        bzero(buffer, 1024);
        n = read(client_socket, buffer, 1023);

        if (n < 0)
        {
            fprintf(stderr, "Failed to read from socket\n");
            continue;
        }
        else if (n == 0)
        {

            /*
                If the client has disconnected from the server, the server should remove the client from the clients array.
                Remove any references to the client from any rooms.
            */

            for (int i = 0; i < num_clients; i++)
            {
                // If the client is found in the clients array
                if (clients[i].socket == client_socket)
                {

                    // Remove the client from any rooms
                    if (strcmp(clients[i].username, "Anonymous") != 0)
                    {
                        pthread_mutex_lock(&rooms_mutex);
                        for (int j = 0; j < num_rooms; j++)
                        {
                            for (int k = 0; k < rooms[j].numberOfPlayers; k++)
                            {
                                if (strcmp(rooms[j].players[k]->client->username, clients[i].username) == 0)
                                {
                                    // Send notification to the room
                                    const char *json = createJsonNotification("LEFT", rooms[j].players[k]->client->username);
                                    struct mcsender *sc = mc_sender_init(NULL, rooms[j].address, SERVER_PORT);
                                    mc_sender_send(sc, json, strlen(json) + 1);
                                    mc_sender_uinit(sc);

                                    fprintf(stderr, "Client %s left room %s\n", rooms[j].players[k]->client->username, rooms[j].name);

                                    pthread_mutex_lock(&rooms[j].mutex);
                                    rooms[j].players[k]->client = NULL;
                                    for (int y = k; y < rooms[j].numberOfPlayers - 1; y++)
                                    {
                                        rooms[j].players[y] = rooms[j].players[y + 1];
                                    }
                                    rooms[j].numberOfPlayers--;
                                    pthread_mutex_unlock(&rooms[j].mutex);
                                    break;
                                }
                            }
                        }
                        pthread_mutex_unlock(&rooms_mutex);
                    }

                    printf("Client disconnected: %s:%d\n", inet_ntoa(clients[i].address.sin_addr),
                           ntohs(clients[i].address.sin_port));

                    // Remove the client from the clients array
                    pthread_mutex_lock(&clients_mutex);
                    for (int j = i; j < num_clients - 1; j++)
                    {
                        clients[j] = clients[j + 1];
                    }
                    pthread_mutex_unlock(&clients_mutex);
                    break;
                }
            }
            pthread_mutex_lock(&clients_mutex);
            num_clients--;
            fprintf(stderr, "Total clients: %d\n", num_clients);
            pthread_mutex_unlock(&clients_mutex);
            break;
        }

        fprintf(stderr, "Received message: %s\n", buffer);

        // If the message is empty, continue to the next iteration
        if (strlen(buffer) == 0)
        {
            continue;
        }

        // If the message is PING, send PONG
        if (strcmp(buffer, "PING") == 0)
        {
            send(client_socket, "PONG", 4, 0);
            fprintf(stderr, "Sent PONG\n");
            continue;
        }

        // If the message is not a JSON string, send an error message
        if (buffer[0] != '{' || buffer[strlen(buffer) - 1] != '}')
        {
            const char *errorMessage = createJsonErrorMessage("Invalid JSON");
            fprintf(stderr, "Invalid JSON\n");
            send(client_socket, errorMessage, strlen(errorMessage), 0);
            continue;
        }

        // Parse the JSON string
        Request *request = parseRequest(buffer);

        if (request->type == NULL)
        {
            const char *errorMessage = createJsonErrorMessage("Invalid JSON");
            fprintf(stderr, "Invalid JSON\n");
            send(client_socket, errorMessage, strlen(errorMessage), 0);
            continue;
        }

        const char *requestType = request->type;

        // Request type sign in
        if (strcmp(requestType, "SIGN_IN") == 0)
        {
            User *user = userParse(request->data);

            // Check if the user exists
            char query[256];
            sprintf(query, "SELECT * FROM users WHERE email = '%s' AND password = '%s'", user->email, user->password);
            PGresult *result = PQexec(conn, query);

            if (result == NULL)
            {
                const char *errorMessage = createJsonErrorMessage("Error executing query");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

            // If the user does not exist, send an error message
            const int rows = PQntuples(result);
            if (rows == 0)
            {
                const char *errorMessage = createJsonErrorMessage("Email or password incorrect");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                PQclear(result);
                continue;
            }
            user->email = strdup(PQgetvalue(result, 0, 0));
            user->password = strdup(PQgetvalue(result, 0, 2));
            user->username = strdup(PQgetvalue(result, 0, 1));
            user->avatar = strtoul(PQgetvalue(result, 0, 3), NULL, 10);

            // Check if the user is already logged in
            for (int j = 0; j < num_clients; j++)
            {
                if (strcmp(clients[j].username, user->username) == 0)
                {
                    const char *errorMessage = createJsonErrorMessage("Already logged in");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                    free(user);
                    free(request);
                    goto continua;
                }
            }

            PQclear(result);

            Client *client = find_client_by_socket(client_socket);
            client->username = user->username;
            client->avatar = user->avatar;

            // Send the user data to the client
            const char *jsonMessage = userToJson(user);
            send(client_socket, jsonMessage, strlen(jsonMessage), 0);

            free(user);
            free(request);

        continua:
            continue;
        }

        // Request type sign up
        if (strcmp(requestType, "SIGN_UP") == 0)
        {
            // Parse the JSON string
            User *user = userParse(request->data);

            // Check if the user exists
            char query[256];
            sprintf(query, "SELECT * FROM users WHERE email = '%s'", user->email);
            PGresult *result = PQexec(conn, query);
            int rows = PQntuples(result);
            if (rows != 0)
            {
                const char *errorMessage = createJsonErrorMessage("Email already exists");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                PQclear(result);
                continue;
            }

            // Check if the username exists
            sprintf(query, "SELECT * FROM users WHERE username = '%s'", user->username);
            result = PQexec(conn, query);
            rows = PQntuples(result);
            if (rows != 0)
            {
                const char *errorMessage = createJsonErrorMessage("Username already exists");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                PQclear(result);
                continue;
            }

            // Insert the user into the database
            sprintf(query, "INSERT INTO users VALUES ('%s', '%s', '%s', %d)", user->email, user->username, user->password, user->avatar);
            result = PQexec(conn, query);
            if (PQresultStatus(result) != PGRES_COMMAND_OK)
            {
                const char *errorMessage = createJsonErrorMessage("Error inserting user into database");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                PQclear(result);
                continue;
            }
            PQclear(result);

            Client *client = find_client_by_socket(client_socket);
            client->username = user->username;
            client->avatar = user->avatar;

            // Send success message
            const char *successMessage = createJsonSuccessMessage("User created successfully");
            send(client_socket, successMessage, strlen(successMessage), 0);
            continue;
        }

        // Request type list of rooms
        if (strcmp(requestType, "LIST_ROOMS") == 0)
        {
            // create JSON message
            const char *jsonMessage = createJsonListOfRooms();
            send(client_socket, jsonMessage, strlen(jsonMessage), 0);
            continue;
        }

        // Request type join room
        if (strcmp(requestType, "JOIN_ROOM") == 0)
        {
            Client *client = find_client_by_socket(client_socket);
            if (client == NULL)
            {
                const char *errorMessage = createJsonErrorMessage("Something went wrong");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

            // Check if the user is logged in
            if (strcmp(client->username, "Anonymous") == 0)
            {
                const char *errorMessage = createJsonErrorMessage("You must be logged in to join a room");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

            // Get the port from the request
            const char *address = json_object_get_string(json_object_object_get(request->data, "address"));
            // Find the room with the port
            Room *room = NULL;
            for (int i = 0; i < num_rooms; i++)
            {
                if (strcmp(rooms[i].address, address) == 0)
                {
                    room = &rooms[i];
                    break;
                }
            }

            // Send error message if room not found
            if (room == NULL)
            {
                const char *errorMessage = createJsonErrorMessage("Room not found");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }
            // Send error message if room is full
            if (room->numberOfPlayers == room->maxPlayers)
            {
                const char *errorMessage = createJsonErrorMessage("Room is full");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }
            const char *successMessage = roomToJson(room);
            send(client_socket, successMessage, strlen(successMessage), 0);
            // Send success message
            fprintf(stderr, "Room joined: %s\n", successMessage);
        }

        // Request type new room
        if (strcmp(requestType, "NEW_ROOM") == 0)
        {
            const Client *client = find_client_by_socket(client_socket);
            if (client == NULL)
            {
                const char *errorMessage = createJsonErrorMessage("Something went wrong");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

            if (strcmp(client->username, "Anonymous") == 0)
            {
                const char *errorMessage = createJsonErrorMessage("You must be logged in to create a room");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
            }
            pthread_mutex_lock(&rooms_mutex);
            // extract_room(request, &rooms[num_rooms]);
            rooms[num_rooms].numberOfPlayers = 0;
            rooms[num_rooms].inGame = false;
            char baseAddress[] = "239.0.0.";
            char address[16];
            sprintf(address, "%s%d", baseAddress, num_rooms + 1);
            rooms[num_rooms].address = address;

            Room *room = roomParse(request->data);
            rooms[num_rooms].name = room->name;
            rooms[num_rooms].maxPlayers = room->maxPlayers;
            rooms[num_rooms].language = room->language;

            Player *player = malloc(sizeof(Player));
            player->client = client;
            player->score = 0;
            player->status = GUESSER;
            rooms[num_rooms].players[0] = player;
            rooms[num_rooms].numberOfPlayers++;
            // create thread for the room
            pthread_t tid;
            rooms[num_rooms].thread = tid;
            pthread_mutex_unlock(&rooms_mutex);

            int err = pthread_create(&tid, NULL, &handle_room, &rooms[num_rooms]);
            if (err != 0)
            {
                const char *errorMessage = createJsonErrorMessage(strerror(err));
                send(client_socket, errorMessage, strlen(errorMessage), 0);
            }
            else
            {
                pthread_mutex_lock(&rooms_mutex);
                num_rooms++;
                pthread_mutex_unlock(&rooms_mutex);
                const char *successMessage = createJsonSuccessMessage(address);
                send(client_socket, successMessage, strlen(successMessage), 0);
            }
            // Clean up
            free(room);
        }
    }

    close(client_socket);
    return NULL;
}