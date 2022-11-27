#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "filefuncs.h"

/**
 * @brief Get the File Stats object
 *  
 * mode_t    st_mode;     protection - S_IFDIR - directory, S_IFREG -reg file
 * uid_t     st_uid;      user ID of owner 
 * off_t     st_size;     total size, in bytes 
 * time_t    st_atime;    time of last access 
 * time_t    st_mtime;    time of last modification 
 * time_t    st_ctime;    time of last status change 
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
    char combined[SIZE] = {0};
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

    // Clear out the data buffer after each loop
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
    char serverRoot[] = "server1Root/";
    // char* combined = (char*)malloc(strlen(serverRoot) + strlen(filePath) + 1);
    char combined[SIZE] = {0};
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
    // strncpy(data, TERMINATE, sizeof(TERMINATE));
    // send(socketFD, data, sizeof(data), 0);
    // bzero(data, SIZE);
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
    struct stat st = {0};
    if (stat("server1Root", &st) == -1) {
        mkdir("server1Root", 0777);
    }

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
    printf("Msg from client: %s\n", client_message);
    printf("Length of message: %ld\n", strlen(client_message));

    FileSystemOp_t* op = parseClientInput(client_message);

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