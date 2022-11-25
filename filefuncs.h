#define SIZE 2000
#define TERMINATE "END##"

typedef struct FileSystemOp {
    char* operation;
    // Array of strings of the file path
    char* filePath;
} FileSystemOp_t;

/**
 * @brief Write the file from the socket descriptor as a stream
 * to the specified file name.
 * 
 * @param sockfd The socket descriptor
 * @param path The file path
 * @return int 
 */
int write_file(int sockDesc, char* filepath);

/**
 * @brief Parse the client's input string into the operation and filepath
 * Format example is "GET folder/text.txt"
 * GET, INFO, PUT, MD, RM
 * 
 * @param input 
 * @return FileSystemOp_t* 
 */
FileSystemOp_t* parseClientInput(char* input);