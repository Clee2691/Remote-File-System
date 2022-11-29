#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "filefuncs.h"

/**
 * @brief Get a file from the remote server
 * GET operation
 * @param socket_desc The socket descriptor
 * @param fileName The file name to get
 * @return int 0 if successful, 1 if failed.
 */
int getFileFromServer(int socket_desc, char** patharr, int pathsize) {
    // Make a downloads folder in the root if it doesn't already exist
    mkdir("clientRoot/downloads", 0777);

    // Combine the root and the file name
    // Download all files into the downloads folder
    char* combined = (char*)calloc(MAXPATHLEN, sizeof(char));
    strcat(combined, CLIENTROOT);
    strcat(combined, "downloads/");
    strcat(combined, patharr[pathsize - 1]);

    // Write the stream to the file path described in input
    int res = write_file(socket_desc, combined);
    free(combined);
    return res; 
}

/**
 * @brief Get information about a file or directory
 * INFO operation
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
    memset(fileInfoBuffer, '\0', sizeof(fileInfoBuffer));
    return 0;
}

/**
 * @brief Put a stream of bytes into the server. Create a file on the server.
 * PUT Operation
 * @param patharr The path array
 * @param psize The path size
 * @param socket_desc The socket
 * @return int 0 if success, 1 for failure
 */
int putBytesToServer(char** patharr, int psize, int socket_desc) {
    // Get the contents to write to the file on the server
    char bytesToPut[SIZE] = {0};
    printf("\nEnter file contents (MAX: 2000 characters):\n");
    fgets(bytesToPut, SIZE, stdin);

    // Send the contents to the server
    if(send(socket_desc, bytesToPut, strlen(bytesToPut), 0) < 0){
        printf("Unable to send request.\n");
        return 1;
    }
    memset(bytesToPut, '\0', sizeof(bytesToPut));

    return 0;
}

/**
 * @brief Make directory or directories on the server
 * MD Operation
 * @param socket_desc The socket
 * @return int 0 for success, 1 for failure
 */
int makeDirOnServer(int socket_desc) {
    char mkdirStatus[SIZE] = {0};
    int res = 0;
    // Receive the server's response
    if(recv(socket_desc, mkdirStatus, sizeof(mkdirStatus), 0) < 0){
        printf("Error while receiving server's msg\n");
        return 1;
    }

    if (strncmp("SUCCESS", mkdirStatus, strlen("SUCCESS")) == 0) {
        printf("Successfully created the directory\n");
        res = 0;
    } else if (strncmp("INVALID", mkdirStatus, strlen("INVALID")) == 0) {
        printf("Invalid folder structure!\n");
        res = 1;
    } else if (strncmp("EXISTS", mkdirStatus, strlen("EXISTS")) == 0) {
        printf("Directory already exists!\n");
        res = 1;
    }

    memset(mkdirStatus, '\0', sizeof(mkdirStatus));
    return res;
}

/**
 * @brief Remove a directory or file on the server
 * RM Operation
 * @param socket_desc The socket descriptor 
 * @return int 0 for success, 1 for failure
 */
int rmDirOnServer(int socket_desc) {
    char rmdirStatus[SIZE] = {0};
    int res = 0;

    // Receive the server's response
    if(recv(socket_desc, rmdirStatus, sizeof(rmdirStatus), 0) < 0){
        printf("Error while receiving server's msg\n");
        return 1;
    }

    if (strncmp("SUCCESS", rmdirStatus, strlen("SUCCESS")) == 0) {
        printf("Successfully removed the directory or file!\n");
        res = 0;
    } else if (strncmp("NOTEMPTY", rmdirStatus, strlen("NOTEMPTY")) == 0) {
        printf("Directory has to be empty to delete it!!\n");
        res = 1;
    } else if (strncmp("NOTFOUND", rmdirStatus, strlen("NOTFOUND")) == 0) {
        printf("Directory or file doesn't exist!\n");
        res = 1;
    } else if (strncmp("BUSY", rmdirStatus, strlen("BUSY")) == 0) {
        printf("Directory or file is in use!\n");
        res = 1;
    }
    
    memset(rmdirStatus, '\0', sizeof(rmdirStatus));
    return res;
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
    
    printf("Socket created successfully.\n");
    
    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Send connection request to server:
    if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        return 1;
    }
    printf("Connected with server successfully.\n");
    FileSystemOp_t* theRequest;

    while(1) {

        // Get input from the user:
        printf("Enter Request: ");
        fgets(client_message, SIZE, stdin);
        // Clear the \n from the end of the client_message
        client_message[strcspn(client_message, "\n")] = 0;
        
        // Send the message to server:
        if(send(socket_desc, client_message, strlen(client_message), 0) < 0){
            printf("Unable to send request.\n");
            return 1;
        }

        if (strncmp(client_message, "exit", strlen("exit")) == 0) {
            break;
        }

        // 2D array for list of strings for the path
        char** patharr = (char**)calloc(MAXPATHLEN, sizeof(char));

        // Parse client input
        theRequest = parseClientInput(client_message, patharr);
        if (theRequest == NULL) {
            printf("Entered unsupported or not correctly formatted request!\n");
        }

        // Do the appropriate operation received from the client
        if (strncmp("GET", theRequest->operation, strlen("GET")) == 0) {
            getFileFromServer(socket_desc, theRequest->pathArray, theRequest->pathSize);
        } else if (strncmp("INFO", theRequest->operation, strlen("INFO")) == 0) {
            getInfoFromServer(theRequest->path, socket_desc);
        } else if (strncmp("PUT", theRequest->operation, strlen("PUT")) == 0) {
            putBytesToServer(theRequest->pathArray, theRequest->pathSize, socket_desc);
        } else if (strncmp("MD", theRequest->operation, strlen("MD")) == 0) {
            makeDirOnServer(socket_desc);
        } else if (strncmp("RM", theRequest->operation, strlen("MD")) == 0) {
            rmDirOnServer(socket_desc);
        }
        
        // Receive the server's response
        if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
            printf("Error while receiving server's msg\n");
            return 1;
        }
        
        printf("Server's response: %s\n",server_message);
        freeOperation(theRequest);
    }

    close(socket_desc);
    return 0;
}