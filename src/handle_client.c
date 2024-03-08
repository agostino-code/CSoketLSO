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

    while (1)
    {
        bzero(buffer, 1024);
        n = read(client_socket, buffer, 1023);

        if (n < 0)
        {
            perror("Errore nella lettura dal socket");
            break;
        }
        else if (n == 0)
        {
            printf("Il client si è disconnesso.\n");
            // Client disconnected, remove it from the clients array
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < num_clients; i++)
            {
                if (clients[i].socket == client_socket)
                {
                    printf("Client disconnected: %s:%hu\n", inet_ntoa(clients[i].address.sin_addr),
                           ntohs(clients[i].address.sin_port));
                    memmove(&clients[i], &clients[i + 1], (num_clients - i - 1) * sizeof(Client));
                    break;
                }
            }
            num_clients--;
            printf("Number of clients: %d\n", num_clients);
            pthread_mutex_unlock(&clients_mutex);
            break;
        }

        printf("Messaggio ricevuto: %s\n", buffer);
        // Il tuo codice per gestire il messaggio...
        // if PING, send PONG

        //buffer vuoto
        if (strlen(buffer) == 0)
        {
            continue;
        }

        if (strcmp(buffer, "PING") == 0)
        {
            send(client_socket, "PONG", 4, 0);
            continue;
        }

        if(buffer[0] != '{' || buffer[strlen(buffer) - 1] != '}')
        {
            const char *errorMessage = createJsonErrorMessage("Invalid JSON");
            send(client_socket, errorMessage, strlen(errorMessage), 0);
            continue;
        }

        Request *request = parseRequest(buffer);

        if (request->type == NULL)
        {
            const char *errorMessage = createJsonErrorMessage("Invalid JSON");
            send(client_socket, errorMessage, strlen(errorMessage), 0);
            continue;
        }

        const char *requestType = request->type;

        if (strcmp(requestType, "SIGN_IN") == 0)
        {
            User *user = userParse(request->data);

            char query[256];
            sprintf(query, "SELECT * FROM users WHERE email = '%s' AND password = '%s'", user->email, user->password);
            PGresult *result = PQexec(conn, query);

            if (result == NULL)
            {
                const char *errorMessage = createJsonErrorMessage("Error executing query");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

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

            PQclear(result);

            Client *client = find_client_by_socket(client_socket);
            client->username = user->username;
            client->avatar = user->avatar;

            const char *jsonMessage = userToJson(user);
            send(client_socket, jsonMessage, strlen(jsonMessage), 0);

            free(user);
            free(request);

            continue;
        }

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

            // success message
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

        if (strcmp(requestType, "JOIN_ROOM") == 0)
        {
            pthread_mutex_lock(&clients_mutex);
            const Client *client = find_client_by_socket(client_socket);
            if (client == NULL)
            {
                const char *errorMessage = createJsonErrorMessage("Something went wrong");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

            if (strcmp(client->username, "Anonymous") == 0)
            {
                const char *errorMessage = createJsonErrorMessage("You must be logged in to join a room");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

            // Get the port from the request
            int port = json_object_get_int(request->data);
            // Find the room with the port
            Room *room = NULL;
            for (int i = 0; i < num_rooms; i++)
            {
                if (rooms[i].port == port)
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

            Player *player = malloc(sizeof(Player));
            player->client = *client;
            player->score = 0;
            if (room->inGame)
            {
                player->status = SPECTATOR;
            }
            else
            {
                player->status = GUESSER;
            }

            room->players[room->numberOfPlayers] = player;
            room->numberOfPlayers++;
            
            // Send success message
            const char *successMessage = roomToJson(room);
            send(client_socket, successMessage, strlen(successMessage), 0);
        }

        if (strcmp(requestType, "NEW_ROOM") == 0)
        {
            pthread_mutex_lock(&clients_mutex);
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

            // extract_room(request, &rooms[num_rooms]);
            rooms[num_rooms].numberOfPlayers = 0;
            rooms[num_rooms].inGame = false;
            rooms[num_rooms].port = htons(3000 + num_rooms+1);

            Room *room = roomParse(request->data);
            rooms[num_rooms].name = room->name;
            rooms[num_rooms].maxPlayers = room->maxPlayers;
            rooms[num_rooms].language = room->language;

            // create thread for the room
            pthread_t tid;
            rooms[num_rooms].thread = tid;
            int err = pthread_create(&tid, NULL, &handle_room, &rooms[num_rooms]);
            if (err != 0)
            {
                const char *errorMessage = createJsonErrorMessage(strerror(err));
                send(client_socket, errorMessage, strlen(errorMessage), 0);
            }
            else
            {
                num_rooms++;
                // Send success message
                char * port = malloc(6);
                sprintf(port, "%d", rooms[num_rooms].port);
                const char *successMessage = createJsonSuccessMessage(port);
                send(client_socket, successMessage, strlen(successMessage), 0);
            }

            // Clean up
            free(room);
            pthread_mutex_unlock(&clients_mutex);
        }
    }

    close(client_socket);
    return NULL;
}