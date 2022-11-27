#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "filefuncs.h"

int parseFilePath(char* fullPath, char* patharr[]) {
    int i = 0;
    char* rest = fullPath;
    // Get the first folder or file
    patharr[i] = strtok_r(rest, "/", &rest);
    // printf("Name %d: %s\n", i, patharr[i]);
    // Keep splitting the file or folder path
    while(patharr[++i] = strtok_r(NULL,"/", &rest)) {
        // printf("Name %d: %s\n", i, patharr[i]);
    }
    return i;
}

FileSystemOp_t* parseClientInput(char* input) {
    FileSystemOp_t* theOperation = (FileSystemOp_t*)malloc(sizeof(FileSystemOp_t));
    // Should be the operation to do like GET or INFO
    char* operation;
    char* rest = input;
    operation = strtok_r(rest, " ", &rest);

    // printf("Compare: %d\n", strcmp("GET", operation));

    // If entered command is not one of the 5 commands, return NULL
    if (!strcmp("GET", operation)  == 0 &&
        !strcmp("INFO", operation) == 0 &&
        !strcmp("PUT", operation)  == 0 &&
        !strcmp("MD", operation)   == 0 &&
        !strcmp("RM", operation)   == 0) {
        return NULL;
    }

    theOperation->operation = operation;

    char* path = strtok_r(rest, " ", &rest);
    theOperation->path = path;

    // Maximum # of names in the path
    int maxNumPathName = 10;
    char* patharr[maxNumPathName];

    // Make a copy of the path to manipulate
    char pathCopy[SIZE] = {0};
    strncpy(pathCopy, theOperation->path, strlen(theOperation->path));
    
    int pathSize = parseFilePath(pathCopy, patharr);

    // Save the file path array and the total path size
    theOperation->pathArray = patharr;
    theOperation->pathSize = pathSize;

    // Check to see that the path size isn't too long
    if (theOperation->pathSize > 10) {
        printf("Path name is too long!\n");
        return NULL;
    }

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
        if(n <= 0) { // || strncmp("END##", buffer, strlen("END##")) == 0) {
            break;
        }
        fprintf(fp, "%s", buffer);
        totalBytes -= strlen(buffer);
        bzero(buffer, SIZE);
        // printf("Bytes left: %d\n", totalBytes);
    }
    fclose(fp);
    return 0;
}

