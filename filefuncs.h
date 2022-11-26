#define SIZE 2000
#define CLIENTROOT "clientRoot/"
#define TERMINATE "END##"

typedef struct FileSystemOp {
    char* operation;
    // Array of strings of the file path
    char* path;
    char** pathArray;
    int pathSize;
} FileSystemOp_t;

/**
 * @brief Parse the file path of the request
 * 
 * @param fullPath The full file path
 * @param patharr The array holding the file path
 * @return int Number of tokens in the file path
 */
int parseFilePath(char* fullPath, char* patharr[]);

/**
 * @brief Parse the client's input string into the operation and filepath
 * Format example is "GET folder/text.txt"
 * GET, INFO, PUT, MD, RM
 * 
 * @param input 
 * @return FileSystemOp_t* object
 */
FileSystemOp_t* parseClientInput(char* input);

/**
 * @brief Write the file from the socket descriptor as a stream
 * to the specified file name.
 * 
 * @param sockfd The socket descriptor
 * @param path The file path
 * @return int 0 If successful, 1 if failed.
 */
int write_file(int sockDesc, char* filepath);