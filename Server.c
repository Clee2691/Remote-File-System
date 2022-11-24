#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define SIZE 2000
#define TERMINATE "END##"

typedef struct FileSystemOp {
    char* operation;
    // Array of strings of the file path
    char* filePath;
} FileSystemOp_t;

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
int sendFile(FILE *fp, int socketFD) {
    struct stat fileStats = getFileStats("./serverRoot/test.txt");

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
 * @brief Parse the client's input string into the operation and filepath
 * Format example is "GET folder/text.txt"
 * GET, INFO, PUT, MD, RM
 * 
 * @param input 
 * @return FileSystemOp_t* 
 */
FileSystemOp_t* parseClientInput(char* input) {
    printf("Parsing.\n");
    FileSystemOp_t* theOperation = malloc(sizeof(FileSystemOp_t));
    // Should be the operation to do like GET or INFO
    char* operation;
    char* rest = input;
    operation = strtok_r(rest, " ", &rest);

    printf("Compare: %d\n", strcmp("GET", operation));

    // If entered command is not one of the 5 commands, return NULL
    if (!strcmp("GET", operation)  == 0 &&
        !strcmp("INFO", operation) == 0 &&
        !strcmp("PUT", operation)  == 0 && 
        !strcmp("MD", operation)   == 0 && 
        !strcmp("RM", operation)   == 0) {
        return NULL;
    }

    theOperation->operation = operation;
    printf("Entered Operation: %s\n", theOperation->operation);

    char* path = strtok_r(rest, " ", &rest);
    theOperation->filePath = path;
    printf("Entered Path: %s\n", theOperation->filePath);
    return theOperation;
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
    
    //TODO: CHANGE THIS
    char* fileName = "./serverRoot/test.txt";
    // The file pointer
    FILE* fp;

    fp = fopen(fileName, "r");
    if (fp == NULL) {
        printf("Error reading the file. Probably wrong path\n");
        exit(1);
    }

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

    // Check if GET, send file if it is
    int res = -1;
    if (strncmp("GET", client_message, strlen("GET")) == 0) {
        res = sendFile(fp, client_sock);
    }

    if (res == -1) {
        printf("Client didn't respond with GET\n");
        message = "Client didn't respond with GET\n";
    } else if (res == 0) {
        printf("Error sending file to client.\n");
        message = "Error sending file to client.";
    } else {
        printf("Successfully sent file to client!\n");
        message = "Successfully sent file to client!";
    }
    
    // Respond to client:
    strcpy(server_message, "Success!");

    if (send(client_sock, server_message, strlen(server_message), 0) < 0){
        printf("Can't send\n");
    }
    printf("Closing connection.\n");
    // Closing the socket:
    close(client_sock);
    close(socket_desc);

    return 0;
}