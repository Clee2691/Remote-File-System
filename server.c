#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "filefuncs.h"

/**
 * @brief Get the file or directory statistics
 * 
 * @param fileName The path to the file
 * @return struct stat Stat object containing file statistics
 */
struct stat getFileStats(char* fileName) {
    struct stat fileStat;
    int fileDesc;

    fileDesc = open(fileName, O_RDONLY);

    if (fstat(fileDesc, &fileStat) < 0)
    {
        printf("Failed getting stats for file: %s.\n", fileName);
        fileStat.st_size = -1;
    }
    close(fileDesc);
    return fileStat;
}

/**
 * @brief Send file or directory statistics to the client
 * 
 * @param filePath The file or directory path
 * @param socketFD The clients socket
 * @return int 0 if successful, 1 if not
 */
int sendFileStat(char* filePath, int socketFD) {
    char infoBuffer[SIZE] = {0};
    char serverRoot[] = "server1Root/";
    char* combined = (char*)malloc(SIZE);
    strcat(combined, serverRoot);
    strncat(combined, filePath, strlen(filePath));

    // Get the file stats
    struct stat fileStat = getFileStats(combined);

    // Is it a directory or a file?
    if (S_ISDIR(fileStat.st_mode)) {
        strcat(infoBuffer, "Type: Directory\n");
    } else if (S_ISREG(fileStat.st_mode)) {
        strcat(infoBuffer, "Type: File\n");
    }

    // The file size
    char size[SIZE] = {0};
    snprintf(size, SIZE, "Size: %ld bytes\n", fileStat.st_size);

    // Concat file size into the buffer
    strncat(infoBuffer, size, strlen(size) + 1);
    strcat(infoBuffer, "Permissions: ");

    char perms[SIZE] = {0};
    // Concat permissions to the permission buffer
    strcat(perms, (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    strcat(perms, (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    strcat(perms, (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    strcat(perms, (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    strcat(perms, (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    strcat(perms, (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    strcat(perms, (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    strcat(perms, (fileStat.st_mode & S_IROTH) ? "r" : "-");
    strcat(perms, (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    strcat(perms, (fileStat.st_mode & S_IXOTH) ? "x\n" : "-\n");

    // Concat all into the buffer
    strncat(infoBuffer, perms, strlen(perms));

    char statusChange[SIZE] = {0};
    char lastFileAccess[SIZE] = {0};
    char lastFileModification[SIZE] = {0};

    // Concat the timing to the buffer
    snprintf(statusChange, SIZE, 
            "Last status change: %s", 
            ctime(&fileStat.st_ctime));

    snprintf(lastFileAccess, SIZE, 
            "Last file access: %s", 
            ctime(&fileStat.st_atime));

    snprintf(lastFileModification, SIZE, 
            "Last file modification: %s\n", 
            ctime(&fileStat.st_mtime));
    
    strncat(infoBuffer, statusChange, strlen(statusChange));
    strncat(infoBuffer, lastFileAccess, strlen(lastFileAccess));
    strncat(infoBuffer, lastFileModification, strlen(lastFileModification));

    // Send info to the client
    if (send(socketFD, infoBuffer, sizeof(infoBuffer), 0) == -1) {
        return 1;
    }

    // Clear out the data buffer
    bzero(infoBuffer, SIZE);
    return 0;
}

/**
 * @brief Send a file through the socket SIZE amounts at a time.
 * 
 * @param fp The file pointer
 * @param socketFD The socket's file descriptor
 * @return int 0 if successful 1 if failed 
 */
int sendFile(char* filePath, int socketFD) {
    // The file pointer
    FILE* fp;
    // The data buffer initialized to 0s
    char data[SIZE] = {0};

    // Combine the root to the filepath
    char* serverRoot = "server1Root/";
    char* combined = (char*)malloc(MAXPATHLEN);
    strcat(combined, serverRoot);
    strncat(combined, filePath, strlen(filePath));

        // Get the file statistics
    struct stat fileStats = getFileStats(combined);

    // Only need the file size to send the correct aomunt of bytes
    int fileSize = fileStats.st_size;

    // Error getting file stats
    if (fileSize == -1) {
        printf("Could not get file data! Probably doesn't exist.\n");
        char* message = "NOTFOUND";
        strcpy(data, message);
        send(socketFD, data, sizeof(data), 0);
        bzero(data, SIZE);
        return 1;
    }

    // If it is not a file, nothing to send
    if (!S_ISREG(fileStats.st_mode)) {
        printf("Not a file!\n");
        char* message = "NOTFILE";
        strcpy(data, message);
        send(socketFD, data, sizeof(data), 0);
        bzero(data, SIZE);
        return 1;
    }

    // Try to open the file
    fp = fopen(combined, "r");
    // No file found
    if (fp == NULL) {
        printf("Error, file at path '%s' not found!\n", combined);
        return 1;
    }

    // Send the file size in bytes
    sprintf(data, "%d", fileSize);
    send(socketFD, data, sizeof(data), 0);
    bzero(data, SIZE);

    // Loop through the file SIZE amounts at a time and
    // send to server
    int size = 0;
    while(fgets(data, SIZE, fp) != NULL) {
        if (send(socketFD, data, sizeof(data), 0) == -1) {
            return 1;
        }
        // Clear out the data buffer after each loop
        bzero(data, SIZE);
    }
    free(combined);
    return 0;
}

/**
 * @brief Make the directory path recursively
 * 
 * @param dirPath The directory path
 */
void mkDirPath(const char* dirPath) {
    char* sep = strrchr(dirPath, '/');
    if(sep != NULL) {
        *sep = 0;
        mkDirPath(dirPath);
        *sep = '/';
    }

    mkdir(dirPath, 0777);
}

/**
 * @brief Write stream of bytes to a file
 * 
 * @param fp The file pointer to write to 
 * @param clientSocket The connected client socket
 * @return int 0 for success, 1 for failure
 */
int writeToFile(FILE* fp, int clientSocket) {
    // Create buffer to receive client input
    char byteBuffer[SIZE] = {0};
    // Get the client's input
    if (recv(clientSocket, byteBuffer, sizeof(byteBuffer), 0) < 0){
        printf("Couldn't receive client message\n");
        return 1;
    }

    // Write to the file
    fprintf(fp, "%s", byteBuffer);

    return 0;
}

/**
 * @brief The PUT command. Makes the directory and the file
 * that is to be written to on the file server
 * 
 * @param pathName The path name
 * @param pathArr The array of path members
 * @param pathSize The path size
 * @param socketFD The socket descriptor
 * @return int 0 for success, 1 for failure
 */
int putBytesToFile(char* pathName, char** pathArr, int pathSize, int socketFD) {
    // Last item in array needs to be a valid file name with an extension
    char* fName = pathArr[pathSize - 1];
    printf("File name: %s\n", fName);

    // Check valid filename
    if (strchr(fName, '.') == NULL) {
        printf("Invalid file name\n");
        return 1;
    }

    // Make the directory path
    char* dirPath = (char*)calloc(MAXPATHLEN, sizeof(char));
    strcat(dirPath, "server1Root/");
    for (int i = 0; i < pathSize - 1; i++) {
        strcat(dirPath, pathArr[i]);
        if (i == pathSize - 2) {
            continue;
        }
        strcat(dirPath, "/");
    }

    mkDirPath(dirPath);
    char* fullPath = (char*)malloc(MAXPATHLEN);
    strcat(fullPath, "server1Root/");
    strcat(fullPath, pathName);

    FILE* fp = fopen(fullPath, "w");
    if (fp == NULL) {
        printf("Error opening file for writing.\n");
        return 1;
    }

    // Write the bytes to the new file.
    int res = writeToFile(fp, socketFD);

    // Close the opened file
    fclose(fp);

    return res;
}

/**
 * @brief The MD command. Makes the directories specified
 * 
 * @param pathArr The path array
 * @param pathSize The number of members in the path
 * @param client_sock The client socket
 * @return int 0 for success, 1 for failure
 */
int makeDirectories(char** pathArr, int pathSize, int client_sock) {
    char status[SIZE] = {0};

    // Check valid filename
    if (strchr(pathArr[pathSize - 1], '.') != NULL) {
        printf("Cannot have '.' in directory name!\n");

        strcpy(status, "INVALID");

        if (send(client_sock, status, strlen(status), 0) < 0){
            printf("Can't send response to client!\n");
        }
        return 1;
    }

    // Make the directory path
    char* dirPath = (char*)calloc(MAXPATHLEN, sizeof(char));
    strcat(dirPath, "server1Root/");
    for (int i = 0; i < pathSize; i++) {
        strcat(dirPath, pathArr[i]);
        if (i == pathSize - 1) {
            continue;
        }
        strcat(dirPath, "/");
    }

    struct stat dstat;

    // Check if directory exists
    if (stat(dirPath, &dstat) == 0 && S_ISDIR(dstat.st_mode)) {
        printf("Directory already exists!\n");

        // Send to client that the directory already exists
        strcpy(status, "EXISTS");
        if (send(client_sock, status, strlen(status), 0) < 0){
            printf("Can't send response to client!\n");
        }

        return 1;
    }

    // Make the directories
    mkDirPath(dirPath);

    // Send success status to the client
    strcpy(status, "SUCCESS");
    if (send(client_sock, status, strlen(status), 0) < 0){
        printf("Can't send response to client!\n");
    }
    return 0;
}

/**
 * @brief The RM command to delete a file or folder
 * 
 * @param path The path to the file or folder
 * @param client_sock The client socket
 * @return int 0 for success, 1 for failure
 */
int delFileOrDir(const char* path, int client_sock) {
    char* finalPath = (char*)malloc(MAXPATHLEN);

    strcat(finalPath, "server1Root/");
    strcat(finalPath, path);

    char status[SIZE] = {0};

    // Remove the file or directory
    int res = remove(finalPath);

    // Return the appropriate response to the attempted deletion
    if (res != 0 && errno == ENOTEMPTY) {
        printf("Directory has to be empty to delete it!\n");
        strcpy(status, "NOTEMPTY");
        if (send(client_sock, status, strlen(status), 0) < 0){
            printf("Can't send response to client!\n");
        }
        return 1;
    } else if (res != 0 && errno == ENOENT) {
        printf("Directory or file not found!\n");
        strcpy(status, "NOTFOUND");
        if (send(client_sock, status, strlen(status), 0) < 0){
            printf("Can't send response to client!\n");
        }
        return 1;
    } else if (res != 0 && errno == EBUSY) {
        printf("File or Directory is in use! Cannot remove!\n");
        strcpy(status, "BUSY");
        if (send(client_sock, status, strlen(status), 0) < 0){
            printf("Can't send response to client!\n");
        }
        return 1;
    }

    printf("Succesfully deleted!\n");
    strcpy(status, "SUCCESS");
    if (send(client_sock, status, strlen(status), 0) < 0){
        printf("Can't send response to client!\n");
    }

    return 0;
}

/**
 * @brief Clean up sockets descriptors and free malloced data
 * 
 * @param op FileSystemOp_t object
 * @param client_sock The client socket
 * @param socket_desc The socket descriptor
 */
void cleanUp(FileSystemOp_t* op, int client_sock, int socket_desc) {
        // Free operation struct
    free(op);
    // Closing the socket
    printf("Closing connection.\n");
    close(client_sock);
    close(socket_desc);
}

/**
 * @brief The entrypoint to the program
 * 
 * @return int 1 or 0
 */
int main (void) {

    // Create a root directory for the server on initial launch if not there
    mkdir("server1Root", 0777);

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
        return 1;
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
        return 1;
    }

    printf("Done with binding\n");

    // Listen for clients:
    if(listen(socket_desc, 1) < 0){
        printf("Error while listening\n");
        return 1;
    }
    printf("\nListening for incoming connections.....\n");
    
    // Accept an incoming connection:
    client_size = sizeof(client_addr);
    client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
    
    if (client_sock < 0){
        printf("Can't accept\n");
        return 1;
    }
    printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    
    //=============================

    //    Get client's action

    //=============================

    if (recv(client_sock, client_message, sizeof(client_message), 0) < 0){
        printf("Couldn't receive client message\n");
        return 1;
    }
    printf("Request from client: %s\n", client_message);

    // 2D array for list of strings for the path
    char** patharr = (char**)malloc(MAXPATHLEN);

    FileSystemOp_t* op = parseClientInput(client_message, patharr);

    char* message;

    // Invalid parsed operation
    if(op == NULL) {
        printf("Invalid client input. Improperly formatted or path name too long!\n");
        message = "Invalid client input. Improperly formatted or path name too long!\n";

        // Copy message into the buffer
        strcpy(server_message, message);

        // Send the response buffer to the client
        if (send(client_sock, server_message, strlen(server_message), 0) < 0){
            printf("Can't send response to client!\n");
        }

        cleanUp(op, client_sock, socket_desc);
        return 1;
    }

    //================================

    // Dispatch appropriate function

    //================================

    int res = -1;

    // Check the operation against supported operations
    if (strncmp("GET", op->operation, strlen("GET")) == 0) {
        printf("GET Request\n");
        res = sendFile(op->path, client_sock);
    } else if (strncmp("INFO", op->operation, strlen("INFO")) == 0) {
        printf("INFO Request\n");
        res = sendFileStat(op->path, client_sock);
    } else if (strncmp("PUT", op->operation, strlen("PUT")) == 0) {
        printf("PUT Request\n");
        res = putBytesToFile(op->path, op->pathArray, op->pathSize, client_sock);
    } else if (strncmp("MD", op->operation, strlen("MD")) == 0) {
        printf("MD Request\n");
        res = makeDirectories(op->pathArray, op->pathSize, client_sock);
    } else if (strncmp("RM", op->operation, strlen("RM")) == 0) {
        printf("RM Request\n");
        res = delFileOrDir(op->path, client_sock);
    } else {
        printf("Unsupported action!\n");
    }

    if (res == -1) {
        printf("Client didn't respond with supported action!\n");
        message = "Unknown request!\n";
    } else if (res == 1) {
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

    cleanUp(op, client_sock, socket_desc);
    return 0;
}