#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "filefuncs.h"

/**
 * @brief Get the File Stats object
 * 
 * @param fileName The path to the file
 * @return struct stat Stat object containing file statistics
 */
struct stat getFileStats(char* fileName) {
    struct stat file_stat;
    int fileDesc;

    fileDesc = open(fileName, O_RDONLY);
    if (fstat(fileDesc, &file_stat) < 0)
    {
        printf("Failed getting file stats.");
        file_stat.st_size = -1;
    }
    // The file size
    printf("File size: %ld\n", file_stat.st_size);
    return file_stat;
}

/**
 * @brief Send a file through the socket SIZE amounts at a time.
 * 
 * @param fp The file pointer
 * @param socketFD The socket's file descriptor
 * @return int 0 if failed 1 if successful 
 */
int sendFile(char* filePath, int socketFD) {
    //TODO: CHANGE THIS
    char* fileName = "./server1Root/test.txt";
    // The file pointer
    FILE* fp;
    // Server root folder
    char serverRoot[] = "server1Root/";

    // Try to open the file
    fp = fopen(fileName, "r");
    if (fp == NULL) {
        printf("Error reading the file. Probably wrong path.\n");
        return 0;
    }
    
    char* combined = strcat(serverRoot, filePath);
    printf("Filepath: %s\n", combined);
    struct stat fileStats = getFileStats(combined);

    int fileSize = fileStats.st_size;
    if (fileSize == -1) {
        printf("Could not get file data!\n");
    }

    // The data buffer initialized to 0s
    char data[SIZE] = {0};

    // Send the file size in bytes
    sprintf(data, "%d", fileSize);
    send(socketFD, data, sizeof(data), 0);
    bzero(data, SIZE);

    // Loop through the file SIZE amounts at a time and
    // send to server
    int size = 0;
    while(fgets(data, SIZE, fp) != NULL) {
        if (send(socketFD, data, sizeof(data), 0) == -1) {
            return 0;
        }
        // Clear out the data buffer after each loop
        bzero(data, SIZE);
    }
    // strncpy(data, TERMINATE, sizeof(TERMINATE));
    // send(socketFD, data, sizeof(data), 0);
    // bzero(data, SIZE);
    return 1;
}

/**
 * @brief The entrypoint to the program
 * 
 * @return int 1 or 0
 */
int main (void) {
    int socket_desc;
    int client_sock;
    int client_size;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    char server_message[SIZE];
    char client_message[SIZE];

    // Clean the char buffers
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));

    // Socket creation
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    // Failed creating a socket
    if(socket_desc < 0){
        printf("Error creating socket\n");
        return -1;
    }

    // Set address and port reuse so that I can continue running this program multiple times
    const int enable = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        printf("setsockopt(SO_REUSEADDR) failed\n");
    }
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
        printf("setsockopt(SO_REUSEPORT) failed\n");
    }

    printf("Socket created successfully\n");

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Couldn't bind to the port\n");
        return -1;
    }

    printf("Done with binding\n");

    // Listen for clients:
    if(listen(socket_desc, 1) < 0){
        printf("Error while listening\n");
        return -1;
    }
    printf("\nListening for incoming connections.....\n");
    
    // Accept an incoming connection:
    client_size = sizeof(client_addr);
    client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
    
    if (client_sock < 0){
        printf("Can't accept\n");
        return -1;
    }
    printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    
    // Receive client's message:
    if (recv(client_sock, client_message, sizeof(client_message), 0) < 0){
        printf("Couldn't receive\n");
        return -1;
    }
    printf("Msg from client: %s\n", client_message);
    printf("Length of message: %ld\n", strlen(client_message));

    FileSystemOp_t* op = parseClientInput(client_message);

    char* message;

    if(op == NULL) {
        printf("Invalid client input. Nothing done!\n");
        message = "Invalid client input. Nothing done!\n";
    }

    //================================

    // Dispatch appropriate function

    //================================

    // Check if GET, send file if it is
    int res = -1;

    if (strncmp("GET", op->operation, strlen("GET")) == 0) {
        printf("GET Request\n");
        res = sendFile(op->filePath, client_sock);
    } else if (strncmp("INFO", op->operation, strlen("INFO"))) {
        printf("INFO Request\n");
    }

    if (res == -1) {
        printf("Client didn't respond with appropriate action!\n");
        message = "Unknown request!\n";
    } else if (res == 0) {
        printf("Error with %s request!\n", op->operation);
        message = "Error with request\n";
    } else {
        printf("Successful %s request!\n", op->operation);
        message = "Successful request!";
    }
    
    // Copy message into the buffer
    strcpy(server_message, message);

    // Send the response buffer to the client
    if (send(client_sock, server_message, strlen(server_message), 0) < 0){
        printf("Can't send response to client!\n");
    }

    // Free operation struct
    free(op);
    // Closing the socket
    printf("Closing connection.\n");
    close(client_sock);
    close(socket_desc);

    return 0;
}