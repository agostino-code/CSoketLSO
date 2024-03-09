FROM ubuntu:latest as build

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake 

# Set the working directory to /app


# Copy the current directory contents into the container at /app
COPY ./CMakeLists.txt /app/CMakeLists.txt
COPY ./lib /app/lib
COPY ./src /app/src

# Create a build directory and set it as the working directory

WORKDIR /app
RUN cmake .

RUN make all

# Expose the port
EXPOSE 3000
EXPOSE 5432

ENTRYPOINT [ "./LSOProject" ]