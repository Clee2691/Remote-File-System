#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "filefuncs.h"

/**
 * @brief Get the File From Server object
 *
 * @param socket_desc The socket descriptor
 * @param fileName The file name to get
 * @return int 0 if successful, 1 if failed.
 */
int getFileFromServer(int socket_desc, char* fileName) {
    // Check if downloads folder exists
    struct stat st = {0};
    if (stat("clientRoot/downloads", &st) == -1) {
        mkdir("clientRoot/downloads", 0777);
    }

    // Combine the root and the file name
    // Download all files into the downloads folder
    char combined[SIZE] = {0};
    strcat(combined, CLIENTROOT);
    strcat(combined, "downloads/");
    strcat(combined, fileName);

    // Write the stream to the file path described in input
    return write_file(socket_desc, combined);
}

/**
 * @brief Get the Info From Server object
 * 
 * @param thePath
 * @param socket_desc 
 * @return int 0 if successful, 1 if failed
 */
int getInfoFromServer(char* thePath, int socket_desc) {
    char fileInfoBuffer[SIZE] = {0};
    // Receive the server's response
    if(recv(socket_desc, fileInfoBuffer, sizeof(fileInfoBuffer), 0) < 0){
        printf("Error while receiving server's msg\n");
        return 1;
    }
    printf("\nInformation:\n\n%s", fileInfoBuffer);
    bzero(fileInfoBuffer, SIZE);
    return 0;
}

/**
 * @brief Clean up sockets and malloced data
 * 
 * @param theRequest 
 * @param socket_desc 
 */
void cleanUp(FileSystemOp_t* theRequest, int socket_desc) {
    // Free the malloced operation struct
    free(theRequest);

    printf("Closing the socket.\n");
    // Close the socket:
    close(socket_desc);
}

/**
 * @brief The entry point for the client
 * 
 * @return int 0 or 1
 */
int main (void) {
    // Create a root directory for the client on initial launch if not there
    struct stat st = {0};
    if (stat("clientRoot", &st) == -1) {
        mkdir("clientRoot", 0777);
    }

    // Start of connection
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
        return 1;
    }
    
    printf("Socket created successfully\n");
    
    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Send connection request to server:
    if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        return 1;
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
        return 1;
    }
    // Parse client input
    FileSystemOp_t* theRequest = parseClientInput(client_message);
    if (theRequest == NULL) {
        printf("Entered unsupported or not correctly formatted request!\n");
        cleanUp(theRequest, socket_desc);
        return 1;
    }

    char fname[SIZE] = {0};
    int arraySize = theRequest->pathSize - 1;
    strncpy(fname, theRequest->pathArray[arraySize], strlen(theRequest->pathArray[arraySize]));

    int res = -1;
    if (strncmp("GET", theRequest->operation, strlen("GET")) == 0) {
        res = getFileFromServer(socket_desc, fname);
    } else if (strncmp("INFO", theRequest->operation, strlen("INFO")) == 0) {
        res = getInfoFromServer(theRequest->path, socket_desc);
    }

    // Was write successful?
    if (res == 0) {
        printf("Successful request!\n");
    } else if (res == 1) {
        printf("Failed request!\n");
    }
    
    // Receive the server's response
    if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
        printf("Error while receiving server's msg\n");
        return 1;
    }
    
    printf("Server's response: %s\n",server_message);

    cleanUp(theRequest, socket_desc);
    return 0;
}