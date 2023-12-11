// Created by: Agostino Cesarano
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <libpq-fe.h>
#include <yyjson.h>

#define SERVER_PORT 3000
#define MAX_CLIENTS 100
#define MAX_ROOMS 50
#define MAX_PLAYERS 20

// Structure to hold client information
typedef struct {
    int socket;
    struct sockaddr_in address;
    char username[256];
} Client;

enum avatars {
    AVATAR_1 = 1,
    AVATAR_2,
    AVATAR_3,
    AVATAR_4,
    AVATAR_5,
    AVATAR_6,
    AVATAR_7,
    AVATAR_8,
    AVATAR_9,
    AVATAR_10,
    AVATAR_11,
    AVATAR_12,
    AVATAR_13,
    AVATAR_14,
    AVATAR_15,
    AVATAR_16
};

enum avatars getAvatarFromNumber(int number) {
    switch (number) {
        case 1:
            return AVATAR_1;
        case 2:
            return AVATAR_2;
        case 3:
            return AVATAR_3;
        case 4:
            return AVATAR_4;
        case 5:
            return AVATAR_5;
        case 6:
            return AVATAR_6;
        case 7:
            return AVATAR_7;
        case 8:
            return AVATAR_8;
        case 9:
            return AVATAR_9;
        case 10:
            return AVATAR_10;
        case 11:
            return AVATAR_11;
        case 12:
            return AVATAR_12;
        case 13:
            return AVATAR_13;
        case 14:
            return AVATAR_14;
        case 15:
            return AVATAR_15;
        case 16:
            return AVATAR_16;
        default:
            // Return a default avatar or handle the error case
            // Here, we return AVATAR_1 as the default
            return AVATAR_1;
    }
}

//enum ResponseType{
//    USER_NOT_FOUND,
//    WRONG_PASSWORD,
//};
//
//const char* getResponseType(enum ResponseType responseType){
//    switch (responseType) {
//        case WRONG_PASSWORD:
//            return "WRONG_PASSWORD";
//        case USER_NOT_FOUND:
//            return "USER_NOT_FOUND";
//    }
//};

typedef struct {
    char email[256];
    char username[256];
    char password[256];
    enum avatars avatar;
} User;

enum player_status {
    SPECTATOR,
    GUESSER,
    CHOOSER
};

enum room_status {
    WAITING,
    INGAME,
    GUESSED,
    CLOSE
};

//enum for max Player 10,15,20
enum max_player {
    TEN = 10,
    FIFTEEN = 15,
    TWENTY = 20
};

enum max_player getMaxPlayerFromNumber(int number) {
    switch (number) {
        case 10:
            return TEN;
        case 15:
            return FIFTEEN;
        case 20:
            return TWENTY;
        default:
            // Return a default maxplayer or handle the error case
            // Here, we return AVATAR_1 as the default
            return TEN;
    }
}

enum language {
    ENGLISH,
    ITALIAN,
    SPANISH,
    GERMAN
};

enum language getLanguageFromString(const char* langString) {
    if (strcmp(langString, "en") == 0) {
        return ENGLISH;
    } else if (strcmp(langString, "it") == 0) {
        return ITALIAN;
    } else if (strcmp(langString, "es") == 0) {
        return SPANISH;
    } else if (strcmp(langString, "de") == 0) {
        return GERMAN;
    } else {
        // Handle unknown language or return a default value
        return ENGLISH; // You can choose a different default if needed
    }
}

typedef struct {
    int score;
    enum player_status status;
    Client client;
} Player;

typedef struct {
    char name[256];
    unsigned long numberOfPlayers;
    enum max_player maxPlayers;
    int round;
    enum language language;
    Player players[MAX_PLAYERS];
    int socket;
    struct sockaddr_in address;
    pthread_t thread;
    enum room_status status;
} Room;

Room rooms[MAX_ROOMS];
int num_rooms = 0;
// Array to store connected clients
Client clients[MAX_CLIENTS];
int num_clients = 0;

// Mutex for synchronizing access to the clients array
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
//Mutex for synchronizing access to the rooms array
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;

//Database connection
PGconn* conn;

// Function for creating a JSON string from a User object
const char* userToJson(const User* userObj) {
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val* root = yyjson_mut_obj(doc);
    yyjson_mut_val* user = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "responseType", "SUCCESS");
    yyjson_mut_obj_add(root, yyjson_mut_strn(doc, "data", 4), user);
    yyjson_mut_obj_add_str(doc, user, "email", userObj->email);
    yyjson_mut_obj_add_str(doc, user, "username", userObj->username);
    yyjson_mut_obj_add_str(doc, user, "password", userObj->password);
    yyjson_mut_obj_add_int(doc, user, "avatar", userObj->avatar);

    const char* json = yyjson_mut_write(doc, 0, NULL);
    //    if (json) {
    //        printf("json: %s\n", json);
    //        free((void *)json);
    //    }

    printf("Response sent: %s\n", json);

    yyjson_mut_doc_free(doc);
    return json;
}

// Function for creating a JSON error message
const char* createJsonErrorMessage(const char* errorMessage) {
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val* root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "responseType", "ERROR");
    yyjson_mut_obj_add_str(doc, root, "data", errorMessage);

    const char* json = yyjson_mut_write(doc, 0, NULL);
    //    if (json) {
    //        printf("json: %s\n", json);
    //        free((void *)json);
    //    }
    printf("Response sent: %s\n", json);

    yyjson_mut_doc_free(doc);
    return json;
}

// Function for creating a JSON success message
const char* createJsonSuccessMessage(const char* successMessage) {
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val* root = yyjson_mut_obj(doc);
    yyjson_mut_val* data = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "responseType", "SUCCESS");
    yyjson_mut_obj_add(root, yyjson_mut_strn(doc, "data", 4), data);
    yyjson_mut_obj_add_str(doc, data, "message", successMessage);

    const char* json = yyjson_mut_write(doc, 0, NULL);
    //    if (json) {
    //        printf("json: %s\n", json);
    //        free((void *)json);
    //    }
    printf("Response sent: %s\n", json);

    yyjson_mut_doc_free(doc);
    return json;
}

const char* createJsonListOfRooms() {
    pthread_mutex_lock(&rooms_mutex);
    // Create a JSON string from the rooms array
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val* root = yyjson_mut_obj(doc);
    yyjson_mut_val* data = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "responseType", "SUCCESS");
    yyjson_mut_obj_add(root, yyjson_mut_strn(doc, "data", 4), data);

    for (int i = 0; i < num_rooms; i++) {
        if (rooms[i].status == WAITING) {
            yyjson_mut_val* room = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, room, "name", rooms[i].name);
            yyjson_mut_obj_add_int(doc, room, "numberOfPlayers", rooms[i].numberOfPlayers);
            yyjson_mut_obj_add_int(doc, room, "maxPlayers", rooms[i].maxPlayers);
            //Status
            switch (rooms[i].status) {
                case WAITING:
                    yyjson_mut_obj_add_str(doc, room, "status", "WAITING");
                    break;
                case INGAME:
                    yyjson_mut_obj_add_str(doc, room, "status", "INGAME");
                    break;
                case GUESSED:
                    yyjson_mut_obj_add_str(doc, room, "status", "GUESSED");
                    break;
                case CLOSE:
                    yyjson_mut_obj_add_str(doc, room, "status", "CLOSE");
                    break;
            }
            switch (rooms[i].language) {
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
    }

    const char* json = yyjson_mut_write(doc, 0, NULL);
    printf("Response sent: %s\n", json);

    yyjson_mut_doc_free(doc);

    pthread_mutex_unlock(&rooms_mutex);
    return json;
}


//function find client by socket
Client* find_client_by_socket(int socket) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].socket == socket) {
            return &clients[i];
        }
    }
    return NULL;
}

//function broadcast to room
void broadcast_to_room(const Room* room, const char* message) {
    for (int i = 0; i < room->numberOfPlayers; i++) {
        send(room->players[i].client.socket, message, strlen(message), 0);
    }
}

void* handle_player(void* arg) {
    const int client_socket = *(int *) arg;
    char request[1024];
    ssize_t num_bytes;

    // Receive and process client requests
    while ((num_bytes = recv(client_socket, request, sizeof(request) - 1, 0)) > 0) {
        request[num_bytes] = '\0';
        printf("Received request: %s\n", request);

        // Extract the request type
        char requestType[256];
        //if request type is message brodcast to all client

        //if request type is quit brodcast to all client your leave

        //if request type is roomlist of all rooms get all room data
    }
    return NULL;
}

//function for handle room
void* handle_room(void* arg) {
    Room* room = (Room *) arg;

    // Create a socket for the room
    const int room_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (room_socket == -1) {
        printf("Could not create socket for room %s\n", room->name);
        return NULL;
    }

    // Bind the socket to the room port

    struct sockaddr_in room_address;
    room_address.sin_family = AF_INET;
    room_address.sin_addr.s_addr = INADDR_ANY;
    room_address.sin_port = htons(3000 + num_rooms);

    if (bind(room_socket, (struct sockaddr *) &room_address, sizeof(room_address)) < 0) {
        printf("Could not bind socket to room %s\n", room->name);
        return NULL;
    }

    // Listen for connections

    if (listen(room_socket, 3) < 0) {
        printf("Could not listen for connections on room %s\n", room->name);
        return NULL;
    }

    // Accept connections from clients

    struct sockaddr_in client_address;
    int client_address_length = sizeof(client_address);

    pthread_mutex_lock(&rooms_mutex);
    room->address = room_address;
    room->socket = room_socket;
    room->thread = pthread_self();
    pthread_mutex_unlock(&rooms_mutex);

    while (room->status != CLOSE) {
        int client_socket = accept(room_socket, (struct sockaddr *) &client_address,
                                   (socklen_t *) &client_address_length);
        if (client_socket < 0) {
            printf("Could not accept client connection on room %s\n", room->name);
            continue;
        }

        // Add the client to the players array
        //        pthread_mutex_lock(&rooms_mutex);
        if (room->numberOfPlayers < room->maxPlayers) {
            const Client* client = find_client_by_socket(client_socket);
            if (strcmp(client->username, "Anonymous") != 0) {
                room->players[room->numberOfPlayers].client = *client;
                room->players[room->numberOfPlayers].score = 0;
                if (room->status == INGAME) {
                    room->players[room->numberOfPlayers].status = SPECTATOR;
                } else {
                    room->players[room->numberOfPlayers].status = GUESSER;
                }
                room->numberOfPlayers++;
                printf("Client connected to room %s\n", room->name);
            } else {
                printf("Client not logged in\n");
            }
        } else {
            printf("Room is full\n");
        }
        //        pthread_mutex_unlock(&rooms_mutex);

        //Broadcast to all clients in the room
        char message[1024];
        sprintf(message, "Client %s:%d connected to room %s\n", inet_ntoa(client_address.sin_addr),
                ntohs(client_address.sin_port), room->name);
        broadcast_to_room(room, message);

        // Create a thread for the client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_player, &client_socket) != 0) {
            printf("Could not create thread for client in room %s\n", room->name);
            continue;
        }

        //        pthread_mutex_lock(&rooms_mutex);
        //        for (int i = 0; i < room->numberOfPlayers; i++) {
        //            if (room->players[i].client.socket == room_socket) {
        //                memmove(&room->players[i], &room->players[i + 1], (room->numberOfPlayers - i - 1) * sizeof(Player));
        //                break;
        //            }
        //        }
        //        room->numberOfPlayers--;
        pthread_mutex_unlock(&rooms_mutex);
    }

    // Close the room socket
    close(room_socket);
    pthread_mutex_lock(&rooms_mutex);
    num_rooms--;
    pthread_mutex_unlock(&rooms_mutex);
    // Clean up the thread resources
    pthread_detach(pthread_self());
}

// Function to handle a connected client
void* handle_client(void* arg) {
    const int client_socket = *((int *) arg);
    char request[1024];
    ssize_t num_bytes;

    // Receive and process client requests
    while ((num_bytes = recv(client_socket, request, sizeof(request) - 1, 0)) > 0) {
        request[num_bytes] = '\0';
        printf("Received request: %s\n", request);

        yyjson_doc* doc = yyjson_read(request, strlen(request), 0);
        // Check if parsing was successful
        if (doc != NULL) {
            // Access the root object
            yyjson_val* root = yyjson_doc_get_root(doc);

            // Access the "requestType" property
            yyjson_val* requestType_value = yyjson_obj_get(root, "requestType");

            const char* requestType = yyjson_get_str(requestType_value);

            if (strcmp(requestType, "SIGN_IN") == 0) {
                yyjson_val* data_value = yyjson_obj_get(root, "data");

                // Access the "email" property
                yyjson_val* email_value = yyjson_obj_get(data_value, "email");

                const char* email = yyjson_get_str(email_value);
                printf("Email: %s\n", email);

                // Access the "password" property
                yyjson_val* password_value = yyjson_obj_get(data_value, "password");

                const char* password = yyjson_get_str(password_value);
                printf("Password: %s\n", password);

                // Release the memory allocated by yyjson
                yyjson_doc_free(doc);

                //Check if the user exists

                char query[256];
                sprintf(query, "SELECT * FROM users WHERE email = '%s' AND password = '%s'", email, password);
                PGresult* result = PQexec(conn, query);
                const int rows = PQntuples(result);
                if (rows == 0) {
                    const char* errorMessage = createJsonErrorMessage("Email or password incorrect");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                    //send(client_socket, "errorMessage", strlen("errorMessage"), 0);
                    continue;
                }

                //Create the user object
                User user;
                strcpy(user.email, PQgetvalue(result, 0, 0));
                strcpy(user.username, PQgetvalue(result, 0, 1));
                strcpy(user.password, PQgetvalue(result, 0, 2));
                user.avatar = strtoul(PQgetvalue(result, 0, 3), NULL, 10);


                Client* client = find_client_by_socket(client_socket);
                strcpy(client->username, user.username);

                PQclear(result);
                //Create the JSON message
                const char* jsonMessage = userToJson(&user);
                send(client_socket, jsonMessage, strlen(jsonMessage), 0);
                continue;
            }

            if (strcmp(requestType, "SIGN_UP") == 0) {
                yyjson_val* data_value = yyjson_obj_get(root, "data");

                yyjson_val* email_value = yyjson_obj_get(data_value, "email");

                const char* email = yyjson_get_str(email_value);

                printf("Email: %s\n", email);

                // Access the "username" property
                yyjson_val* username_value = yyjson_obj_get(data_value, "username");

                const char* username = yyjson_get_str(username_value);
                printf("Username: %s\n", username);

                // Access the "password" property
                yyjson_val* password_value = yyjson_obj_get(data_value, "password");

                const char* password = yyjson_get_str(password_value);
                printf("Password: %s\n", password);

                // Access the "avatar" property
                yyjson_val* avatar_value = yyjson_obj_get(data_value, "avatar");

                const int avatar = yyjson_get_int(avatar_value);
                printf("Avatar: %d\n", avatar);

                // Release the memory allocated by yyjson
                yyjson_doc_free(doc);

                //Check if the user exists
                char query[256];
                sprintf(query, "SELECT * FROM users WHERE email = '%s'", email);
                const PGresult* result = PQexec(conn, query);
                int rows = PQntuples(result);
                if (rows != 0) {
                    const char* errorMessage = createJsonErrorMessage("Email already exists");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                    continue;
                }

                sprintf(query, "SELECT * FROM users WHERE username = '%s'", username);
                result = PQexec(conn, query);
                rows = PQntuples(result);
                if (rows != 0) {
                    const char* errorMessage = createJsonErrorMessage("Username already exists");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                    continue;
                }

                //Insert the user into the database
                sprintf(query, "INSERT INTO users VALUES ('%s', '%s', '%s', %d)", email, username, password, avatar);
                result = PQexec(conn, query);
                if (PQresultStatus(result) != PGRES_COMMAND_OK) {
                    const char* errorMessage = createJsonErrorMessage("Error inserting user into database");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                    continue;
                }

                //success message
                const char* successMessage = createJsonSuccessMessage("User created successfully");
                send(client_socket, successMessage, strlen(successMessage), 0);
                continue;
            }

            //Request type list of rooms
            if (strcmp(requestType, "LIST_ROOM") == 0) {
                //create JSON message
                const char* jsonMessage = createJsonListOfRooms();
                send(client_socket, jsonMessage, strlen(jsonMessage), 0);
                continue;
            }

            if (strcmp(requestType, "JOIN_ROOM") == 0) {
                pthread_mutex_lock(&clients_mutex);
                const Client* client = find_client_by_socket(client_socket);
                if (client == NULL) {
                    const char* errorMessage = createJsonErrorMessage("Something went wrong");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                    continue;
                }

                if (strcmp(client->username, "Anonymous") == 0) {
                    const char* errorMessage = createJsonErrorMessage("You must be logged in to join a room");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                    continue;
                }
            }


            if (strcmp(requestType, "NEW_ROOM") == 0) {
                pthread_mutex_lock(&clients_mutex);
                const Client* client = find_client_by_socket(client_socket);
                if (client == NULL) {
                    const char* errorMessage = createJsonErrorMessage("Something went wrong");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                    continue;
                }

                if (strcmp(client->username, "Anonymous") == 0) {
                    const char* errorMessage = createJsonErrorMessage("You must be logged in to create a room");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                    continue;
                }

                //extract_room(request, &rooms[num_rooms]);
                rooms[num_rooms].numberOfPlayers = 0;
                rooms[num_rooms].status = WAITING;

                yyjson_val* data_value = yyjson_obj_get(root, "data");

                // Access the "name" property
                yyjson_val* name_value = yyjson_obj_get(data_value, "name");
                strcpy(rooms[num_rooms].name, yyjson_get_str(name_value));
                printf("Room Name: %s\n", rooms[num_rooms].name);

                // Access the "maxPlayers" property
                yyjson_val* maxPlayers_value = yyjson_obj_get(data_value, "maxPlayers");
                rooms[num_rooms].maxPlayers = getMaxPlayerFromNumber(yyjson_get_int(maxPlayers_value));
                printf("Room MaxPlayers: %u\n", rooms[num_rooms].maxPlayers);
                // Access the "language" property
                yyjson_val* language_value = yyjson_obj_get(data_value, "language");
                rooms[num_rooms].language = getLanguageFromString(yyjson_get_str(language_value));
                printf("Room Language: %u\n", rooms[num_rooms].language);

                // Release the memory allocated by yyjson
                yyjson_doc_free(doc);

                //create thread for the room
                pthread_t tid;
                if (pthread_create(&tid, NULL, &handle_room, &rooms[num_rooms]) != 0) {
                    const char* errorMessage = createJsonErrorMessage("Something went wrong");
                    send(client_socket, errorMessage, strlen(errorMessage), 0);
                }

                num_rooms++;

                pthread_mutex_unlock(&clients_mutex);

                const char* successMessage = createJsonSuccessMessage("Room created successfully");
                send(client_socket, successMessage, strlen(successMessage), 0);
            }
        } else {
            printf("Failed to parse JSON string.\n");
            const char* successMessage = createJsonErrorMessage("Failed to parse JSON string.");
            send(client_socket, successMessage, strlen(successMessage), 0);
            continue;
        }
    }
    // Client disconnected, remove it from the clients array
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].socket == client_socket) {
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
}

int main() {
    //Connect to database
    conn = PQconnectdb("host=localhost port=5432 dbname=postgres user=postgres password=default");
    if (PQstatus(conn) == CONNECTION_BAD) {
        puts("We were unable to connect to the database");
        exit(0);
    } else {
        puts("Database connected");
    }

    /* Connessione al Database */

    //Create table if not exists
    char const query[256] = "CREATE TABLE IF NOT EXISTS users (\n"
            "    email VARCHAR(255) PRIMARY KEY,\n"
            "    username VARCHAR(255) UNIQUE,\n"
            "    password VARCHAR(255),\n"
            "    avatar INT\n"
            ")";
    const PGresult* res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        puts("Error creating table users\n");
    } else {
        puts("Table users created\n");
    }

    /* Crea la tabella utente */

    // int server_socket;
    struct sockaddr_in server_addr;

    // Create the server socket
    int const server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        return 1;
    }

    // Set up server details
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind the server socket to a specific address and port
    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) < 0) {
        perror("Listening failed");
        return 1;
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    // Accept and handle incoming client connections
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // Accept a new client connection
        int client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Add the new client to the clients array
        pthread_mutex_lock(&clients_mutex);
        if (num_clients < MAX_CLIENTS) {
            clients[num_clients].socket = client_socket;
            clients[num_clients].address = client_addr;
            strcpy(clients[num_clients].username, "Anonymous");
            printf("Client connected: %s:%hu\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            num_clients++;
            printf("Number of clients: %d\n", num_clients);
        }
        pthread_mutex_unlock(&clients_mutex);

        // Create a new thread to handle the client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, &client_socket) != 0) {
            perror("Failed to create thread");
            break;
        }
    }

    // Close the server socket
    close(server_socket);

    return 0;
}
