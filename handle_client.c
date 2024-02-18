#include <unistd.h>
#include <arpa/inet.h>
#include "data/request.h"
#include "globals.h"
#include "json/formatter.h"
#include "handle_client.h"
#include "handle_room.h"

void *handle_client(void *arg)
{
    const int client_socket = *((int *)arg);
    char* bytes;
    ssize_t num_bytes;

    // Receive and process client requests
    while ((num_bytes = recv(client_socket, bytes, sizeof(bytes) - 1, 0)) > 0)
    {
        bytes[num_bytes] = '\0';
        printf("Received request: %s\n", bytes);

        // Parse the JSON string
        Request *request = parseRequest(bytes);

        if (request == NULL)
        {
            printf("Not valid request\n");
            const char *successMessage = createJsonErrorMessage("Not valid request");
            send(client_socket, successMessage, strlen(successMessage), 0);
            continue;
        }

        char *requestType = request->type;

        if (strcmp(requestType, "SIGN_IN") == 0)
        {
            User *user = userParse(request->data);

            char query[256];
            sprintf(query, "SELECT * FROM users WHERE email = '%s' AND password = '%s'", user->email, user->password);
            PGresult *result = PQexec(conn, query);
            const int rows = PQntuples(result);
            if (rows == 0)
            {
                const char *errorMessage = createJsonErrorMessage("Email or password incorrect");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                // send(client_socket, "errorMessage", strlen("errorMessage"), 0);
                continue;
            }

            strcpy(user->email, PQgetvalue(result, 0, 0));
            strcpy(user->username, PQgetvalue(result, 0, 1));
            strcpy(user->password, PQgetvalue(result, 0, 2));
            user->avatar = strtoul(PQgetvalue(result, 0, 3), NULL, 10);

            Client *client = find_client_by_socket(client_socket);
            strcpy(client->username, user->username);
            client->avatar = user->avatar;

            PQclear(result);
            // Create the JSON message
            const char *jsonMessage = userToJson(user);
            send(client_socket, jsonMessage, strlen(jsonMessage), 0);
            continue;
        }

        if (strcmp(requestType, "SIGN_UP") == 0)
        {
            // Parse the JSON string
            User *user = userParse(request->data);

            // Check if the user exists
            char query[256];
            sprintf(query, "SELECT * FROM users WHERE email = '%s'", user->email);
            const PGresult *result = PQexec(conn, query);
            int rows = PQntuples(result);
            if (rows != 0)
            {
                const char *errorMessage = createJsonErrorMessage("Email already exists");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

            sprintf(query, "SELECT * FROM users WHERE username = '%s'", user->username);
            result = PQexec(conn, query);
            rows = PQntuples(result);
            if (rows != 0)
            {
                const char *errorMessage = createJsonErrorMessage("Username already exists");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

            // Insert the user into the database
            sprintf(query, "INSERT INTO users VALUES ('%s', '%s', '%s', %d)", user->email, user->username, user->password, user->avatar);
            result = PQexec(conn, query);
            if (PQresultStatus(result) != PGRES_COMMAND_OK)
            {
                const char *errorMessage = createJsonErrorMessage("Error inserting user into database");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
                continue;
            }

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
            int port = atoi(request->data);

            // Find the room with the port
            const Room *room = NULL;
            for (int i = 0; i < num_rooms; i++)
            {
                if (rooms[i].address.sin_port == port)
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
            // Get all r
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
            rooms[num_rooms].status = WAITING;

            // Set the room address
            rooms[num_rooms].address.sin_family = AF_INET;
            rooms[num_rooms].address.sin_addr.s_addr = INADDR_ANY;
            rooms[num_rooms].address.sin_port = htons(3000 + num_rooms);

            Room *room = roomParse(request->data);
            // create thread for the room
            pthread_t tid;
            if (pthread_create(&tid, NULL, &handle_room, &rooms[num_rooms]) != 0)
            {
                const char *errorMessage = createJsonErrorMessage("Something went wrong");
                send(client_socket, errorMessage, strlen(errorMessage), 0);
            }
            else
            {
                num_rooms++;
                // Send success message
                char *port = malloc(6);
                sprintf(port, "%d", 3000 + num_rooms);
                const char *successMessage = createJsonSuccessMessage(port);
                send(client_socket, successMessage, strlen(successMessage), 0);
            }
            pthread_mutex_unlock(&clients_mutex);
        }
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

        // Close the client socket
        close(client_socket);

        // Clean up the thread resources
        pthread_detach(pthread_self());
        return 0;
    }
    return 0;
}