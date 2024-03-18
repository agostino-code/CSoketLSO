/*
    Created by: Agostino Cesarano
    Date: 2024-03-18
    Description: This is the main file of the server application.
    It sets up a server socket that listens for incoming connections on a specific port.
    When a new client connects to the server, a new thread is created to handle the client.
    The server uses the PostgreSQL database to store user information.
*/

#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "config.h"
#include "globals.h"
#include "handle_client.h"

int main()
{

    /*
        Connection to a PostgreSQL database using the libpq library.
        The `PQconnectdb` function returns a `PGconn` object which represents the connection to the database.
        This object is stored in the `conn` variable.

        Next, the `PQstatus` function is called with `conn` as its argument to check the status of the connection.
        If the status equals `CONNECTION_BAD`, this means the connection to the database could not be established.
    */

    conn = PQconnectdb("host=localhost port=5432 dbname=postgres user=postgres password=default");
    if (PQstatus(conn) == CONNECTION_BAD)
    {
        puts("We were unable to connect to the database");
        exit(0);
    }
    else
    {
        puts("Database connected");
    }

    /*
        Create a new table in the database if it doesn't already exist.
        The `query` variable is a constant character array that holds the SQL command to create a new table named `users`.ù
        This table has four columns: `email`, `username`, `password`, and `avatar`.

        The `email` column is the primary key, meaning it must contain unique values.
        The `username` column is also set to be unique.
        The `password` column holds the user's password, and the `avatar` column is an integer,
         possibly used to reference an image file for the user's avatar.
    */

    char const query[256] = "CREATE TABLE IF NOT EXISTS users (\n"
                            "    email VARCHAR(255) PRIMARY KEY,\n"
                            "    username VARCHAR(255) UNIQUE,\n"
                            "    password VARCHAR(255),\n"
                            "    avatar INT\n"
                            ")";
    const PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        puts("Error creating table users\n");
    }
    else
    {
        puts("Table users created\n");
    }

    /*
        Set up a server socket that listens for incoming connections on a specific port.
        The `socket` function is called to create a new socket.
    */

    struct sockaddr_in server_addr;

    int const server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Failed to create socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    /*
        The `setsockopt` function is then called to allow the reuse of the local address.
        This is useful in development as it allows you to restart your server without waiting for the operating system to release your server's socket.
    */

    int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    /*
        The `bind` function is then called to bind the server socket to the specified address and port.
    */

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        fprintf(stderr, "Failed to bind to port %d\n", SERVER_PORT);
        fprintf(stderr, "Port %d might be in use\n", SERVER_PORT);
        return 1;
    }

    /*
        The `listen` function is then called to listen for incoming connections on the server socket.
    */

    if (listen(server_socket, 5) < 0)
    {
        fprintf(stderr, "Failed to listen on port %d\n", SERVER_PORT);
        return 1;
    }

    fprintf(stderr, "Server started on port %d\n", SERVER_PORT);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        /*
            The `accept` function is then called to accept a new client connection.
            This function blocks the program until a new client connects to the server.
        */

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0)
        {
            fprintf(stderr, "Failed to accept client connection\n");
            continue;
        }

        // Add the new client to the clients array
        pthread_mutex_lock(&clients_mutex);
        if (num_clients < MAX_CLIENTS)
        {
            clients[num_clients].socket = client_socket;
            clients[num_clients].address = client_addr;
            // strcpy(clients[num_clients].username, "Anonymous");
            clients[num_clients].username = "Anonymous";
            fprintf(stderr, "Client address: %s\n", inet_ntoa(client_addr.sin_addr));
            num_clients++;
            fprintf(stderr, "Client connected, total clients: %d\n", num_clients);
        }
        pthread_mutex_unlock(&clients_mutex);

        // Create a new thread to handle the new client
        pthread_t tid;
        if (pthread_create(&tid, NULL, &handle_client, &client_socket) != 0)
        {
            fprintf(stderr, "Failed to create thread\n");
            continue;
        }

        // Detach the thread
        pthread_detach(tid);
    }

    // Close the server socket
    close(server_socket);

    return 0;
}
