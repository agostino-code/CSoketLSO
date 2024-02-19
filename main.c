// Created by: Agostino Cesarano
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "config.h"
#include "globals.h"
#include "handle_client.h"

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

        // Create a new thread to handle the new client
        pthread_t tid;
        if (pthread_create(&tid, NULL, &handle_client, &client_socket) != 0) {
            perror("Failed to create thread");
            continue;
        }

        // Detach the thread
        pthread_detach(tid);

    }

    // Close the server socket
    close(server_socket);

    return 0;
}
