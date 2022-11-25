#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "filefuncs.h"

int write_file(int sockDesc, char* filepath) {
    int n; 
    FILE *fp;
    int totalBytes;
    char buffer[SIZE];

    // Get the file size. Server should send this first
    recv(sockDesc, buffer, SIZE, 0);
    // Parse to an integer
    int fSize = atoi(buffer);
    printf("File Size: %d\n", fSize);
    // Clear out the buffer
    bzero(buffer, SIZE);

    // Open file for writing
    fp = fopen(filepath, "w");

    // Error opening file to write
    if(fp==NULL) {
        return 0;
    }
    // Loop and write bytes to the file as big as file size
    totalBytes = fSize;
    while(totalBytes > 0) {
        n = recv(sockDesc, buffer, SIZE, 0);
        if(n <= 0) { // || strncmp("END##", buffer, strlen("END##")) == 0) {
            break;
        }
        fprintf(fp, "%s", buffer);
        totalBytes -= strlen(buffer);
        bzero(buffer, SIZE);
        // printf("Bytes left: %d\n", totalBytes);
    }
    fclose(fp);
    return 1;
}

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
    // printf("Entered Operation: %s\n", theOperation->operation);

    char* path = strtok_r(rest, " ", &rest);
    theOperation->filePath = path;
    // printf("Entered Path: %s\n", theOperation->filePath);
    return theOperation;
}