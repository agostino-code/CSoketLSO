# CSoketLSO

C code Server Socket for Lab of operating systems

- [x] Task create a socket system Guess the word


**C Repository with Docker Compose for PostgreSQL Database**

**Prerequisites**

* **Docker:** Ensure you have Docker installed on your system. Refer to the official documentation for installation instructions: [https://docs.docker.com/engine/install/](https://docs.docker.com/engine/install/)
* **docker-compose:** Install docker-compose using the following command:

```bash
sudo apt-get install docker-compose  # For Debian/Ubuntu-based systems
```

**Third-party libraries**

**libpq**: It allows connect to, communicate with, and execute queries on a PostgreSQL database.

**json-c**: It provides functions for parsing, manipulating, and generating JSON objects and arrays.

**multicast**: This term refers to a one-to-many communication channel where a message is sent to a group of receivers simultaneously.

**Project Structure**
```
CSocketLSO/
|-- CMakeCache.txt
|-- docker-compose.yml
|-- lib
|   |-- json-c
|   |-- libpq-standalone
|   `-- multicast
`-- src
    |-- config.h
    |-- data
    |   |-- client.h
    |   |-- player.c
    |   |-- player.h
    |   |-- request.h
    |   |-- response.h
    |   |-- room.c
    |   |-- room.h
    |   `-- user.h
    |-- globals.c
    |-- globals.h
    |-- handle_client.c
    |-- handle_client.h
    |-- handle_room.c
    |-- handle_room.h
    |-- json
    |   |-- formatter.c
    |   `-- formatter.h
    `-- main.c
```
**Explanation**

* `CMakeLists.txt`: The top-level CMake project file that specifies the project's build configuration.
* `README.md`: Project documentation file.
* `lib`: Directory for third-party libraries.
    * `libpq`: Subdirectory for the PostgreSQL client library (`libpq`).
    * `json-c`: Subdirectory for the JSON manipulation library (`json-c`).
    * `multicast`: Subdirectory for the Multicast manipulation library.
* `src`: Directory for source code (.c ) files.
    * `main.c`: The main program file that contains the entry point for your application.
    * `utils.c`: File for utility functions used by your project.
    * `...`: Other source code.

**Benefits of this Structure**

* **Separation of Concerns:** This structure separates third-party libraries from your project's source code, making it easier to manage and maintain.
* **Code Reusability:** Placing third-party libraries in a dedicated directory makes it easy to reuse them across multiple projects.
* **Clearer Organization:** The structure clearly defines where different types of files should be placed, improving project organization.

**Build and Run**

1. Navigate to the root directory of your project (`CSockerLSO`).
2. Build the Docker image and start the services using:

   ```bash
   docker-compose up -d
   ```

   * `-d`: Runs the services in detached mode, allowing them to run in the background.

**Connect to the Database (Optional)**

1. Use a PostgreSQL client tool like `psql` to connect to the database:

   ```bash
   docker-compose exec db psql -h postgres -U postgres -p 5432
   ```

   * Replace `5432` with the actual port mapped for the database service if you've changed it in `docker-compose.yml`.

## CMake Requirements and Steps for Building and Running C Code

### Prerequisites

Before using CMake, ensure you have the following prerequisites:

1. **CMake:** Install CMake using the appropriate method for your operating system. You can find download and installation instructions on the official CMake website: [https://cmake.org/download/](https://cmake.org/download/)

2. **C Compiler:** Install a C compiler such as GCC or Clang. These compilers are typically available through your system's package manager.

3. **Build Tools:** Ensure you have basic build tools installed, such as `make` or `Ninja`. These tools are often included in the default toolchain of your operating system.

### Building with CMake

Once you have the prerequisites, follow these steps to build your C project using CMake:

1. **Run CMake:**

   ```bash
   cmake .
   ```

   ```bash
   cmake --build .
   ```
2. **Run Application:**
   ```bash
   ./LSOProject
   ```
## Contributors

- **Jonathan Borrelli**: Contributed to the Android part of the project.
- **Agostino Cesarano**: Contributed to the server part of the project.

**Related Pepository**

( https://github.com/agostino-code/LSOProject/ )
