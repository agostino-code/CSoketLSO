FROM ubuntu:latest as build

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
# Set the working directory to /app
WORKDIR /app

# Copy the current directory contents into the container at /app
COPY ./CMakeLists.txt /app/CMakeLists.txt
COPY ./lib /app/lib
COPY ./src /app/src

# Create a build directory and set it as the working directory
RUN mkdir build
WORKDIR /app/build

# Run cmake to build the project
RUN cmake ..

# Run make to build the project
RUN make

# Start a new stage
FROM ubuntu:latest

# Set the working directory to /app
WORKDIR /app

# Run the app when the container launches
CMD ["./build/LSOProject"]