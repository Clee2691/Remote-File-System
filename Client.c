#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define SIZE 2000
#define TERMINATE "END##"

/**
 * @brief Write the file from the socket descriptor as a stream
 * To the specified file name
 * 
 * @param sockfd The socket descriptor
 * @return int 
 */
int write_file(int sockDesc, char* path) {
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
    fp = fopen(path, "w");

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
        printf("Bytes left: %d\n", totalBytes);
    }
    return 1;
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

    int res = write_file(socket_desc, "./clientRoot/test.txt");

    if (res == 1) {
        printf("Successfully written the file.\n");
    } else if (res == 0) {
        printf("Failed writing file.\n");
    }
    
    // Receive the server's response:
    if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
        printf("Error while receiving server's msg\n");
        return -1;
    }
    
    printf("Server's response: %s\n",server_message);

    printf("Closing the socket.\n");
    // Close the socket:
    close(socket_desc);
    return 0;
}