#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "filefuncs.h"

int parseFilePath(char* fullPath, char** patharr) {
    int i = 0;
    char* rest = fullPath;

    // Get the first folder or file
    patharr[i] = strtok_r(rest, "/", &rest);
    // Keep splitting the file or folder path
    while(patharr[++i] = strtok_r(NULL,"/", &rest)) {
    }
    return i;
}

FileSystemOp_t* parseClientInput(char* input, char** patharr) {
    FileSystemOp_t* theOperation = (FileSystemOp_t*)malloc(sizeof(FileSystemOp_t));
    // Should be the operation to do like GET or INFO
    // Max length of operations is 4 but I allocate 10 to be sure
    char* operation = (char*)malloc(10);
    char* rest = input;
    strcpy(operation, strtok_r(rest, " ", &rest));

    // If entered command is not one of the 5 commands, return NULL
    if (!strcmp("GET", operation)  == 0 &&
        !strcmp("INFO", operation) == 0 &&
        !strcmp("PUT", operation)  == 0 &&
        !strcmp("MD", operation)   == 0 &&
        !strcmp("RM", operation)   == 0) {
        return NULL;
    }

    theOperation->operation = operation;
    
    char* path = (char*)calloc(MAXPATHLEN, sizeof(char));
    strcpy(path, strtok_r(rest, " ", &rest));
    theOperation->path = path;

    // Make a copy of the path to manipulate
    char* pathCopy = (char*)calloc(MAXPATHLEN, sizeof(char));
    strncpy(pathCopy, theOperation->path, strlen(theOperation->path));
    
    int pathSize = parseFilePath(pathCopy, patharr);
    
    // Check to see that the path size isn't too long
    if (theOperation->pathSize > 10) {
        printf("Path name is too long!\n");
        return NULL;
    }

    // Save the file path array and the total path size
    theOperation->pathArray = patharr;
    theOperation->pathSize = pathSize;

    return theOperation;
}

int write_file(int sockDesc, char* filepath) {
    int n; 
    FILE *fp;
    int totalBytes;
    char buffer[SIZE];
    int fSize;

    // Get the file size. Server should send this first
    recv(sockDesc, buffer, SIZE, 0);

    // If server responds with NOTFOUND
    if (strcmp(buffer, "NOTFOUND") == 0) {
        printf("File not found on server!\n");
        return 1;
    } else if (strncmp(buffer, "NOTFILE", strlen("NOTFILE")) == 0) {
        printf("Path is not a file!\n");
        return 1;
    // Parse to an integer
    } else {
        fSize = atoi(buffer);
    }
    
    printf("File Size: %d\n", fSize);
    // Clear out the buffer
    bzero(buffer, SIZE);

    // Open file for writing
    fp = fopen(filepath, "w");

    // Error opening file to write
    if(fp == NULL) {
        return 1;
    }

    // Loop and write bytes to the file as big as file size
    totalBytes = fSize;
    while(totalBytes > 0) {
        n = recv(sockDesc, buffer, SIZE, 0);
        if(n <= 0) {
            break;
        }
        fprintf(fp, "%s", buffer);
        totalBytes -= strlen(buffer);
        bzero(buffer, SIZE);
    }
    fclose(fp);
    return 0;
}

