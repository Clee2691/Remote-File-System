#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "filefuncs.h"

int getFileFromServer(char* thePath, int socket_desc) {
    // Root of the client file system
    char cliRoot[] = "clientRoot/";
    // Combine the root and the input path
    char* combined = (char*)malloc(strlen(cliRoot) + strlen(thePath) + 1); 
    combined = strcat(cliRoot, thePath);
    printf("Combined file path: %s\n", combined);

    // Write the stream to the file path described in input
    int res = write_file(socket_desc, "clientRoot/test.txt");

    return 0;
}

int main (void) {
    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[SIZE];
    char client_message[SIZE];
    
    // Clean buffers:
    memset(server_message,'\0',sizeof(server_message));
    memset(client_message,'\0',sizeof(client_message));
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0){
        printf("Unable to create socket\n");
        return -1;
    }
    
    printf("Socket created successfully\n");
    
    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Send connection request to server:
    if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");
    
    // Get input from the user:
    printf("Enter message: ");
    fgets(client_message, SIZE, stdin);
    // Clear the \n from the end of the client_message
    client_message[strcspn(client_message, "\n")] = 0;
    
    // Send the message to server:
    if(send(socket_desc, client_message, strlen(client_message), 0) < 0){
        printf("Unable to send message\n");
        return -1;
    }
    // Parse client input
    FileSystemOp_t* theRequest = parseClientInput(client_message);
    printf("Request: %s\nOperation: %s\n", theRequest->operation, theRequest->filePath);

    
    // TODO: IF ELSE TO BRANCH TO DIFFERENT FUNCTIONS FOR THE
    // TODO: DIFFERENT METHODS
    int res = -1;
    if (strncmp("GET", theRequest->operation, strlen("GET")) == 0) {
        res = getFileFromServer(theRequest->filePath, socket_desc);
    }

    // Root of the client file system
    // char cliRoot[] = "clientRoot/";
    // // Combine the root and the input path
    // char* combined= strcat(cliRoot, theRequest->filePath);
    // printf("Combined file path: %s\n", combined);

    // // Write the stream to the file path described in input
    // int res = write_file(socket_desc, combined);

    // Was write successful?
    if (res == 1) {
        printf("Successfully written the file.\n");
    } else if (res == 0) {
        printf("Failed writing file.\n");
    }
    
    // Receive the server's response
    if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
        printf("Error while receiving server's msg\n");
        return -1;
    }
    
    printf("Server's response: %s\n",server_message);

    // Free the malloced operation struct
    free(theRequest);

    printf("Closing the socket.\n");
    // Close the socket:
    close(socket_desc);
    return 0;
}