#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "filefuncs.h"

char* USB1PATH;
char* USB2PATH;

int USB1AVAIL = 0;
int USB2AVAIL = 0;

/**
 * @brief Check USB 1 and USB 2 drives.
 * Set the global variables if drives exist.
 * 
 * @return int 0 if at least 1 usb drive is present, otherwise 1 if nothing
 * is available
 */
int checkUSB() {
    // The USB drives mounted
    char* usb1 = "/mnt/d/";
    char* usb2 = "/mnt/e/";

    USB1PATH = (char*)calloc(strlen(usb1) + sizeof(char), sizeof(char));
    USB2PATH = (char*)calloc(strlen(usb2) + sizeof(char), sizeof(char));

    struct stat sb;
    // USB Drive #1
    if (stat(usb1, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        printf("USB 1 is available!\n");
        strncpy(USB1PATH, usb1, strlen(usb1));
        USB1AVAIL = 1;
    } else {
        printf("USB 1 not available!\n");
        USB1AVAIL = 0;
    }

    // USB Drive #2
    if (stat(usb2, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        printf("USB 2 is available!\n");
        strncpy(USB2PATH, usb2, strlen(usb2));
        USB2AVAIL = 1;
    } else {
        printf("USB 2 not available!\n");
        USB2AVAIL = 0;
    }

    // No USB drives available for storage
    if (USB1AVAIL == 0 && USB2AVAIL == 0) {
        return 1;
    }
    // At least one drive is available
    return 0;
}

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
    // Check if file is existent
    if (fstat(fileDesc, &fileStat) < 0)
    {   
        // Set the file size to -1 if not found and return the stat
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
    checkUSB();
    char infoBuffer[SIZE] = {0};
    char* combined1 = (char*)calloc(SIZE, sizeof(char));
    char* combined2 = (char*)calloc(SIZE, sizeof(char));
    struct stat fileStat;

     // Both are available
    if (USB1AVAIL == 1 && USB2AVAIL == 1) {
        // USB 1
        strcat(combined1, USB1PATH);
        strncat(combined1, filePath, strlen(filePath));
        fileStat = getFileStats(combined1);

        // If USB 1 doesn't have it, try USB 2
        if (fileStat.st_size == -1) {
            strcat(combined2, USB2PATH);
            strncat(combined2, filePath, strlen(filePath));
            fileStat = getFileStats(combined2);
        }

    // USB 1 is available but # 2 is not
    } else if (USB1AVAIL == 1 && USB2AVAIL == 0) {
        // USB 1
        strcat(combined1, USB1PATH);
        strncat(combined1, filePath, strlen(filePath));
        fileStat = getFileStats(combined1);

    // USB 2 is available but #1 is not
    } else if (USB1AVAIL == 0 && USB2AVAIL == 1) {
        // USB 2
        strcat(combined2, USB2PATH);
        strncat(combined2, filePath, strlen(filePath));
        fileStat = getFileStats(combined2);
    }

    if (fileStat.st_size == -1) {
        printf("File not found on server!\n");

        strcat(infoBuffer, "File not found!\n");

        // Send info to the client
        if (send(socketFD, infoBuffer, sizeof(infoBuffer), 0) == -1) {
            printf("Failed sending info to client.\n");
            return 1;
        }

        // Clear out the data buffer
        memset(infoBuffer, '\0', sizeof(infoBuffer));
        return 1;
    }

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
        printf("Failed sending info to client.\n");
        return 1;
    }

    // Clear out the data buffer
    memset(infoBuffer, '\0', sizeof(infoBuffer));
    free(combined1);
    free(combined2);
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
    checkUSB();
    // The file pointer
    FILE* fp;
    // The data buffer initialized to 0s
    char data[SIZE] = {0};

    // Combine the root to the filepath
    char* combined1 = (char*)calloc(MAXPATHLEN, sizeof(char));
    char* combined2 = (char*)calloc(MAXPATHLEN, sizeof(char));

    struct stat fileStat;

     // Both are available
    if (USB1AVAIL == 1 && USB2AVAIL == 1) {
        // USB 1
        strcat(combined1, USB1PATH);
        strncat(combined1, filePath, strlen(filePath));
        fileStat = getFileStats(combined1);

        // If USB 1 doesn't have it, try USB 2
        if (fileStat.st_size == -1) {
            strcat(combined2, USB2PATH);
            strncat(combined2, filePath, strlen(filePath));
            fileStat = getFileStats(combined2);
            combined1 = NULL;
        }
        combined2 = NULL;

    // USB 1 is available but # 2 is not
    } else if (USB1AVAIL == 1 && USB2AVAIL == 0) {
        // USB 1
        strcat(combined1, USB1PATH);
        strncat(combined1, filePath, strlen(filePath));
        fileStat = getFileStats(combined1);
        combined2 = NULL;

    // USB 2 is available but #1 is not
    } else if (USB1AVAIL == 0 && USB2AVAIL == 1) {
        // USB 2
        strcat(combined2, USB2PATH);
        strncat(combined2, filePath, strlen(filePath));
        fileStat = getFileStats(combined2);
        combined1 = NULL;
    }

    // Error getting file stats
    if (fileStat.st_size == -1) {
        printf("Could not get file data! Probably doesn't exist.\n");
        char* message = "NOTFOUND";
        strcpy(data, message);
        send(socketFD, data, sizeof(data), 0);
        memset(data, '\0', sizeof(data));
        return 1;
    }

    // If it is not a file, nothing to send
    if (!S_ISREG(fileStat.st_mode)) {
        printf("Not a file!\n");
        char* message = "NOTFILE";
        strcpy(data, message);
        send(socketFD, data, sizeof(data), 0);
        memset(data, '\0', sizeof(data));
        return 1;
    }

    // Try to open the file
    if (combined1 != NULL) {
        fp = fopen(combined1, "r");
    } else if (combined2 != NULL) {
        fp = fopen(combined2, "r");
    }
    
    // No file found
    if (fp == NULL) {
        printf("Error, file not found!\n");
        return 1;
    }

    // Send the file size in bytes
    sprintf(data, "%ld", fileStat.st_size);
    send(socketFD, data, sizeof(data), 0);
    memset(data, '\0', sizeof(data));

    // Loop through the file SIZE amounts at a time and
    // send to server
    int size = 0;
    while(fgets(data, SIZE, fp) != NULL) {
        if (send(socketFD, data, sizeof(data), 0) == -1) {
            return 1;
        }
        // Clear out the data buffer after each loop
        memset(data, '\0', sizeof(data));
    }
    free(combined1);
    free(combined2);
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
int writeToFile(FILE* fp1, FILE* fp2, int clientSocket) {
    // Create buffer to receive client input
    char byteBuffer[SIZE] = {0};
    // Get the client's input
    if (recv(clientSocket, byteBuffer, sizeof(byteBuffer), 0) < 0){
        printf("Couldn't receive client message\n");
        return 1;
    }

    // Write to both file pointers if available
    if (fp1 != NULL) {
        fprintf(fp1, "%s", byteBuffer);
    }

    if (fp2 != NULL) {
        fprintf(fp2, "%s", byteBuffer);
    }
    memset(byteBuffer, '\0', sizeof(byteBuffer));
    
    return 0;
}

/**
 * @brief Make the full file path with the path array and an offset
 * along with the USB location
 * 
 * @param finalPath The final path pointer
 * @param pathArr The operation's path array
 * @param pathSize The path array's size
 * @param usb The USB path
 * @param offset The offset
 */
void makePathWithArr(char* finalPath, char** pathArr, int pathSize, char* usb, int offset) {
    // The USB path either 1 or 2
    strcat(finalPath, usb);
    // Concat the array with the usb path
    for (int i = 0; i < pathSize - (offset - 1); i++) {
        strcat(finalPath, pathArr[i]);
        if (i == pathSize - offset) {
            continue;
        }
        strcat(finalPath, "/");
    }
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
    checkUSB();
    // Last item in array needs to be a valid file name with an extension
    char* fName = pathArr[pathSize - 1];
    printf("File name: %s\n", fName);

    // Check valid filename
    if (strchr(fName, '.') == NULL) {
        printf("Invalid file name\n");
        return 1;
    }

    int res;

    // Make the directory path
    char* dirPath1 = (char*)calloc(MAXPATHLEN, sizeof(char));
    char* dirPath2 = (char*)calloc(MAXPATHLEN, sizeof(char));

    // The path to the file
    char* fullPath1 = (char*)calloc(MAXPATHLEN, sizeof(char));
    char* fullPath2 = (char*)calloc(MAXPATHLEN, sizeof(char));

    // The file pointer
    FILE* fp1;
    FILE* fp2;

    // Both are available
    if (USB1AVAIL == 1 && USB2AVAIL == 1) {
        makePathWithArr(dirPath1, pathArr, pathSize, USB1PATH, 2);
        makePathWithArr(dirPath2, pathArr, pathSize, USB2PATH, 2);

        // Make directory path first
        mkDirPath(dirPath1);
        mkDirPath(dirPath2);

        // Make the file path1
        strcat(fullPath1, USB1PATH);
        strcat(fullPath1, pathName);

        // Make the file path2
        strcat(fullPath2, USB2PATH);
        strcat(fullPath2, pathName);

        // Open file pointers
        fp1 = fopen(fullPath1, "w");
        fp2 = fopen(fullPath2, "w");

        if (fp1 == NULL && fp2 == NULL) {
            printf("Error opening file for writing.\n");
            return 1;
        }

        res = writeToFile(fp1, fp2, socketFD);

    // USB 1 is available but # 2 is not
    } else if (USB1AVAIL == 1 && USB2AVAIL == 0) {
        makePathWithArr(dirPath1, pathArr, pathSize, USB1PATH, 2);
        mkDirPath(dirPath1);

        strcat(fullPath1, USB1PATH);
        strcat(fullPath1, pathName);

        fp1 = fopen(fullPath1, "w");

        if (fp1 == NULL) {
            printf("Error opening file for writing.\n");
            return 1;
        }

        res = writeToFile(fp1, NULL, socketFD);

    // USB 2 is available but #1 is not
    } else if (USB1AVAIL == 0 && USB2AVAIL == 1) {
        makePathWithArr(dirPath2, pathArr, pathSize, USB2PATH, 2);
        mkDirPath(dirPath2);

        strcat(fullPath2, USB2PATH);
        strcat(fullPath2, pathName);

        fp2 = fopen(fullPath2, "w");

        if (fp2 == NULL) {
            printf("Error opening file for writing.\n");
            return 1;
        }

        res = writeToFile(NULL, fp2, socketFD);
    }

    // Close the opened file
    if (fp1 != NULL) {
        fclose(fp1);
    }
    if (fp2 != NULL) {
        fclose(fp2);
    }

    free(dirPath1);
    free(dirPath2);
    free(fullPath1);
    free(fullPath2);

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
    checkUSB();
    char status[SIZE] = {0};

    // Check valid directory name
    if (strchr(pathArr[pathSize - 1], '.') != NULL) {
        printf("Cannot have '.' in directory name!\n");

        strcpy(status, "INVALID");

        if (send(client_sock, status, strlen(status), 0) < 0){
            printf("Can't send response to client!\n");
        }
        memset(status, '\0', sizeof(status));
        return 1;
    }
    
    struct stat dstat;
    // Make the directory path
    char* dirPath1 = (char*)calloc(MAXPATHLEN, sizeof(char));
    char* dirPath2 = (char*)calloc(MAXPATHLEN, sizeof(char));

    // Both are available
    if (USB1AVAIL == 1 && USB2AVAIL == 1) {
        makePathWithArr(dirPath1, pathArr, pathSize, USB1PATH, 1);
        makePathWithArr(dirPath2, pathArr, pathSize, USB2PATH, 1);

        // Check if directory exists
        if (stat(dirPath1, &dstat) == 0 && S_ISDIR(dstat.st_mode)) {
            printf("Directory already exists!\n");

            // Send to client that the directory already exists
            strcpy(status, "EXISTS");
            if (send(client_sock, status, strlen(status), 0) < 0){
                printf("Can't send response to client!\n");
            }
            memset(status, '\0', sizeof(status));
            return 1;
        }
        mkDirPath(dirPath1);
        mkDirPath(dirPath2);

    // USB 1 is available but # 2 is not
    } else if (USB1AVAIL == 1 && USB2AVAIL == 0) {
        makePathWithArr(dirPath1, pathArr, pathSize, USB1PATH, 1);

        // Check if directory exists
        if (stat(dirPath1, &dstat) == 0 && S_ISDIR(dstat.st_mode)) {
            printf("Directory already exists!\n");

            // Send to client that the directory already exists
            strcpy(status, "EXISTS");
            if (send(client_sock, status, strlen(status), 0) < 0){
                printf("Can't send response to client!\n");
            }
            memset(status, '\0', sizeof(status));
            return 1;
        }
        mkDirPath(dirPath1);

    // USB 2 is available but #1 is not
    } else if (USB1AVAIL == 0 && USB2AVAIL == 1) {
        makePathWithArr(dirPath2, pathArr, pathSize, USB2PATH, 1);
        // Check if directory exists
        if (stat(dirPath2, &dstat) == 0 && S_ISDIR(dstat.st_mode)) {
            printf("Directory already exists!\n");

            // Send to client that the directory already exists
            strcpy(status, "EXISTS");
            if (send(client_sock, status, strlen(status), 0) < 0){
                printf("Can't send response to client!\n");
            }
            memset(status, '\0', sizeof(status));
            return 1;
        }

        mkDirPath(dirPath2);
    }

    // Send success status to the client
    strcpy(status, "SUCCESS");
    if (send(client_sock, status, strlen(status), 0) < 0){
        printf("Can't send response to client!\n");
    }

    memset(status, '\0', sizeof(status));
    free(dirPath1);
    free(dirPath2);
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
    checkUSB();
    char* finalPath1 = (char*)calloc(MAXPATHLEN, sizeof(char));
    char* finalPath2 = (char*)calloc(MAXPATHLEN, sizeof(char));

    int res;

    char status[SIZE] = {0};

    // Both are available
    if (USB1AVAIL == 1 && USB2AVAIL == 1) {
        // USB 1
        strcat(finalPath1, USB1PATH);
        strcat(finalPath1, path);
        // USB 2
        strcat(finalPath2, USB2PATH);
        strcat(finalPath2, path);

        res = remove(finalPath1);
        res = remove(finalPath2);

    // USB 1 is available but # 2 is not
    } else if (USB1AVAIL == 1 && USB2AVAIL == 0) {
        // USB 1
        strcat(finalPath1, USB1PATH);
        strcat(finalPath1, path);
        
        res = remove(finalPath1);

    // USB 2 is available but #1 is not
    } else if (USB1AVAIL == 0 && USB2AVAIL == 1) {
        // USB 2
        strcat(finalPath2, USB2PATH);
        strcat(finalPath2, path);
        
        res = remove(finalPath2);
    }

    // Return the appropriate response to the attempted deletion
    if (res != 0 && errno == ENOTEMPTY) {
        printf("Directory has to be empty to delete it!\n");
        strcpy(status, "NOTEMPTY");
        if (send(client_sock, status, strlen(status), 0) < 0){
            printf("Can't send response to client!\n");
        }
        memset(status, '\0', sizeof(status));
        return 1;
    } else if (res != 0 && errno == ENOENT) {
        printf("Directory or file not found!\n");
        strcpy(status, "NOTFOUND");
        if (send(client_sock, status, strlen(status), 0) < 0){
            printf("Can't send response to client!\n");
        }
        memset(status, '\0', sizeof(status));
        return 1;
    } else if (res != 0 && errno == EBUSY) {
        printf("File or Directory is in use! Cannot remove!\n");
        strcpy(status, "BUSY");
        if (send(client_sock, status, strlen(status), 0) < 0){
            printf("Can't send response to client!\n");
        }
        memset(status, '\0', sizeof(status));
        return 1;
    }

    printf("Succesfully deleted!\n");
    strcpy(status, "SUCCESS");
    if (send(client_sock, status, strlen(status), 0) < 0){
        printf("Can't send response to client!\n");
    }
    memset(status, '\0', sizeof(status));
    free(finalPath1);
    free(finalPath2);

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
    freeOperation(op);
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
int main () {
    int pid;
    int isFileStoreAvail = checkUSB();
    // No file storage so exit.
    if (isFileStoreAvail == 1) {
        printf("No file storage available. Server failed to start. Exiting.\n");
        return 1;
    } else {
        printf("At least 1 file storage device is available. Starting server...\n");
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

    FileSystemOp_t* op;

    while(1) {
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

        printf("Client connected at IP: %s and port: %i\n", 
                inet_ntoa(client_addr.sin_addr), 
                ntohs(client_addr.sin_port));
        
        pid = fork();

        if (pid == 0) {
            // If it is a child, close the server socket
            close(socket_desc);
            //=============================

            //    Get client's action

            //=============================
            while (1) {
                // Get the initial client request
                if (recv(client_sock, client_message, sizeof(client_message), 0) < 0){
                    printf("Couldn't receive client message\n");
                    return 1;
                }
                printf("Request from client: %s\n", client_message);
                
                if (strncmp(client_message, "exit", strlen("exit")) == 0) {
                    break;
                }

                // 2D array for list of strings for the path
                char** patharr = (char**)calloc(MAXPATHLEN, sizeof(char));

                op = parseClientInput(client_message, patharr);

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
                    memset(server_message, '\0', sizeof(server_message));
                    memset(client_message, '\0', sizeof(client_message));
                    // cleanUp(op, client_sock, socket_desc);
                    continue;
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
                memset(server_message, '\0', sizeof(server_message));
                memset(client_message, '\0', sizeof(client_message));
            }
            close(client_sock);
            break;
        }
    }

    free(USB1PATH);
    free(USB2PATH);

    cleanUp(op, client_sock, socket_desc);
    return 0;
}