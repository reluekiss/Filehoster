#include <stdio.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>

#define ERROR_FILE    0
#define REG_FILE      1
#define DIRECTORY     2
#define DEFAULT_MIME_TYPE "application/octet-stream"
#define MIN(x, y) ((x) < (y) ? (x) : (y))

int typeOfFile(char *filepath);

void handleGET(char *fileToSend, int sock, char *webDir);
void handlePOST(char *buffer, char *filename, int *sock, char *web_dir);

void extractFileRequest(char *method, char *req, char *buff);
void handleOpenFile(const char *filepath, int *fileHandle, off_t *file_size); 
void sendFile(int *sock, char *buffer, int *fileHandle, off_t *file_size);

char *mime_type_get(char *filename);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 +  Function: main()
 +  Description: Program entry point.
 + 
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int main(int argc, char **argv) {
    pid_t pid;            /* pid of child */
    int sockFD;           /* our initial socket */

    int newSockFD;
    int port;             /* Port number, used by 'bind' */
    char webDir[128];    /* Your environment that contains your web webDir*/

    socklen_t clientAddressLength;

    // Struct used for bind and accept.
    struct sockaddr_in serverAddress, clientAddress;

    char fileRequest[256];    /* where we store the requested file name */
    
    // Check for correct program inputs.
    if (argc != 3) {
        fprintf(stderr, "USAGE: %s <port number> <website directory>\n", argv[0]);
        exit(-1);
    }

    port = atoi(argv[1]);       /* Get the port number */
    strcpy(webDir, argv[2]);    /* Get user specified web content directory */


    if ((pid = fork()) < 0) {
        perror("Cannot fork (for deamon)");
        exit(0);
    }
    else if (pid != 0) {
        /*
         * I am the parent
         */
        char t[128];
        sprintf(t, "echo %d > %s.pid\n", pid, argv[0]);
        system(t);
        exit(0);
    }

    /*
     * Create our socket, bind it, listen.
     * This socket will be used for communication
     * between the client and this server.
     */

    //Create
    if((sockFD = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("something went wrong at creating our socket.");
        exit(0);
    }
    //Bind
    serverAddress.sin_family = AF_INET; /* Internet domain */
    serverAddress.sin_addr.s_addr = INADDR_ANY; /* local machine IP address */
    serverAddress.sin_port = htons(port);
    if((bind(sockFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) == -1){
        perror("something went wrong at binding socket.");
        exit(0);
    }
    //Listen
    int backlog = 5;
    if((listen(sockFD,backlog)) == -1){
        perror("something went wrong at listening.");
        exit(0);
    }

    signal(SIGCHLD, SIG_IGN);  /* Prevent zombie processes */

    /*
     * Get the size of the clientAddress structure, could pass NULL if we
     * don't care about who the client is.
     */
    clientAddressLength = sizeof(clientAddress);

    /*
     * - accept a new connection and fork.
     * - If you are the child process,  process the request and exit.
     * - If you are the parent close the socket and come back to
     *   accept another connection
     */

    while (1) {

        /*
         * Accept a connection from a client (a web browser)
         * accept the new connection. newSockFD will be used for the
         * child to communicate to the client (browser)
         */

        if((newSockFD = accept(sockFD, (struct sockaddr *) &clientAddress, &clientAddressLength)) == -1){
            perror("something went wrong at accepting a connection");
            exit(0);
        }

        // Forking off a new process to handle a new connection.
        if ((pid = fork()) < 0) {
            perror("Failed to fork a child process");
            exit(0);
        }
        else if (pid == 0) {
            /*
             * I am the child
             */
            char buff[1024];
            char ref[1024];
            char method[10];

            close(sockFD);
            memset(buff, 0, 1024);
            memset(ref, 0, 1024);

            // Read client request into buff.
            if(read(newSockFD, ref, 1024) == -1){
                perror("something went wrong reading client request");
            }
            memmove(buff, ref, 1024);
            
            // Extract user requested file name.
            printf("client request:\n %s\n", ref);
            
            extractFileRequest(method, fileRequest, buff);
            if (strcmp(method, "GET") == 0) {
                handleGET(fileRequest, newSockFD, webDir);
            }
            else if (strcmp(method, "POST") == 0) { 
                handlePOST(buff, fileRequest, &newSockFD, webDir);
            }
            else {
                int fileHandle;
                off_t file_size;
                char filepath[10];
                char Header[1024];
                sprintf(filepath, "%s/%s", webDir, "a/405.jpg");
                handleOpenFile(filepath, &fileHandle, &file_size);
                sprintf(Header, "HTTP/1.1 405 Method Not Allowed\r\n"
                                "Content-type: image/jpg\r\n"
                                "Content-length: %ld\r\n\r\n", file_size);
                //Send header.
                if( write(newSockFD, Header, strlen(Header)) == -1){
                    perror("Something went wrong writing header.");
                    exit(-1);
                }
                sendFile(&newSockFD, buff, &fileHandle, &file_size);
            }

            shutdown(newSockFD, 1);
            close(newSockFD);
            exit(0);
        } // pid = 0
        else {
            /*
             * I am the Parent
             */
            close(newSockFD);    /* Parent handed off this connection to its child,
			                        doesn't care about this socket */
        }
    }
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 +  Function: typeOfFile()
 +  Description: Tells us if the request is a environment or a regular file
 + 
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int typeOfFile(char *filepath) {
    struct stat buf;    /* to stat the file/dir */

    if (stat(filepath, &buf) != 0) {
        perror("Failed to identify specified file");
        fprintf(stderr, "[ERROR] stat() on file: |%s|\n", filepath);
        fflush(stderr);
        return (ERROR_FILE);
    }

    printf("File full path: %s, file length is %li bytes\n", filepath, buf.st_size);

    if (S_ISREG(buf.st_mode))
        return (REG_FILE);
    else if (S_ISDIR(buf.st_mode))
        return (DIRECTORY);

    return (ERROR_FILE);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 + Function: extractFileRequest()
 + Description:  Extract the file request from the request lines the client
 +               sent to us.  Make sure you NULL terminate your result.
 + 
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void extractFileRequest(char *method, char *req, char *buff) {
  size_t method_len = strcspn(buff, " ");
  strncpy(method, buff, method_len);
  method[method_len] = '\0';

  // Skip over whitespace between the method and requested file.
  size_t i = method_len;
  while (isspace(buff[i])) {
    i++;
  }

  // Find the end of the requested file.
  size_t req_len = strcspn(buff + i, " ");
  strncpy(req, buff + i, req_len);
  req[req_len] = '\0';
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 + Open a file and handle size calculations.
 +
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void handleOpenFile(const char *filepath, int *fileHandle, off_t *file_size) {
    if ((*fileHandle = open(filepath, O_RDONLY)) == -1) {
        fprintf(stderr, "File does not exist: %s\n", filepath);
    }

    *file_size = lseek(*fileHandle, (off_t)0, SEEK_END);
    lseek(*fileHandle, (off_t)0, SEEK_SET);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 +  Function: send/receiveFile()
 +  Description: Send/receive the requested file.
 + 
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void sendFile(int *sock, char *buffer, int *fileHandle, off_t *file_size) {
    ssize_t s;
    int sendbuffer = 0;

    while(sendbuffer <= *file_size) {
        sendbuffer = sendbuffer + 1024;
        //read file
        if((s = read(*fileHandle, buffer, 1024)) == -1){
            perror("Something went wrong reading to buffer.");
            exit(-1);
        }
        //send file
        if((write(*sock, buffer, s)) == -1){
            perror("Something went wrong writing file.");
            exit(-1);
        }
    }
    close(*fileHandle);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 +  Function: handleGET()
 +  Description: Send the requested file.
 + 
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void handleGET(char *fileToSend, int sock, char *webDir) {
    char filepath[256];
    char Header[1024];
    char buffer[1024];
    off_t file_size;
    int fileHandle;
    int fileType;
    char *extension;
    char *mime;
    
    printf("File Requested: |%s|\n", fileToSend);
    fflush(stdout);

    // Build the full path to the file
    sprintf(filepath, "%s/%s", webDir, fileToSend);

    /*
     * - If the requested file is a environment (directory), append 'index.html'
     *   file to the end of the filepath
     * - If the requested file is a regular file, do nothing and proceed
     */

    fileType = typeOfFile(filepath);

    if(fileType == DIRECTORY){
        sprintf(filepath, "%s%s", filepath, "index.html");
    }
    
    int fileToSendLength = strlen(fileToSend);
    if(fileToSendLength > 3){
        extension = &fileToSend[strlen(fileToSend) - 3];
    }
    else
        extension = "html";

    if(fileType == DIRECTORY || fileType == REG_FILE){        
        handleOpenFile(filepath, &fileHandle, &file_size);
        mime = mime_type_get(filepath);
        sprintf(Header, "HTTP/1.0 200 OK\r\n"
                        "Content-type: %s\r\n"
                        "Content-length: %ld\r\n\r\n", mime, file_size);
    }
    else{
        sprintf(filepath, "%s/%s", webDir, "a/404.jpg");
        handleOpenFile(filepath, &fileHandle, &file_size);
        sprintf(Header, "HTTP/1.1 404 Not Found\r\n"
                        "Content-type: image/jpg\r\n"
                        "Content-length: %ld\r\n\r\n", file_size);
    }
     //send header
    if( write(sock, Header, strlen(Header)) == -1){
        perror("Something went wrong writing header.");
        exit(-1);
    }

    sendFile(&sock, buffer, &fileHandle, &file_size);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 + Return a MIME type for a given filename
 +
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char *mime_type_get(char *filename) {
    char *ext = strrchr(filename, '.');

    if (ext == NULL) {
        return DEFAULT_MIME_TYPE;
    }
    
    for (char *p = ext; *p != '\0'; p++) {
        *p = tolower(*p);
    }

    // TODO: this is O(n) and it should be O(1) (Current attempt in wip/mimehash.c)

    if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) { return "text/html"; }
    if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0) { return "image/jpg"; }
    if (strcmp(ext, "css") == 0) { return "text/css"; }
    if (strcmp(ext, "js") == 0) { return "application/javascript"; }
    if (strcmp(ext, "json") == 0) { return "application/json"; }
    if (strcmp(ext, "txt") == 0) { return "text/plain"; }
    if (strcmp(ext, "gif") == 0) { return "image/gif"; }
    if (strcmp(ext, "png") == 0) { return "image/png"; }

    return DEFAULT_MIME_TYPE;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 + Handle a Post request and save the file.
 +
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void handlePOST(char *buffer, char *filename, int *sock, char *web_dir) {
    int i,j = 0;
    // Parse the Content-Length header to determine the size of the request body.
    char *content_length_str = strstr(buffer, "Content-Length: ") + 16;
    long content_length = atoi(strstr(content_length_str, "\r\n") ? content_length_str : strstr(content_length_str, "\n"));
    
    // Write a random file name in the image directory.
    char *ext = strrchr(filename, '.');
    char filepath[2048];
    snprintf(filepath, 2048, "%sdata/%s", web_dir, filename /*randString(16), ext*/);
    if (strlen(filepath) >= 2048) {
        printf("Error: file path too long\n");
        return;
    }
    
    // Parse the body of the request. 
    char *start = strstr(buffer, "\r\n\r\n");
    char *data = malloc(strlen(start) + 1);
    memmove(data, start, strlen(start));
    data[strlen(start)] = '\0';
    
    // Open the file to write to.
    FILE *file;
    file = fopen(filepath, "wb");
    if (file == NULL) {
        printf("Failed to open %s\n", filepath);
    }
    // Write the request body from the buffer.
    int data_size = sizeof(data);
    fwrite(data, data_size, content_length, file);
    
    // Write any more data that isn't in the buffer.
    size_t remaining_bytes = content_length - data_size;
    int bytes_received;
    int wbuffer[8192];
    while (remaining_bytes > 0) {
        bytes_received = read(*sock, wbuffer, MIN(8192, remaining_bytes));
        if (bytes_received == 0) {
            break; // EOF
        }
        else if (bytes_received < 0) {
            perror("Error reading from socket.");
            break;
        }
        fwrite(wbuffer, 1, bytes_received, file);
        remaining_bytes -= bytes_received;
    }

    fclose(file);
    free(data);
    
    // Send back a header.
    char Header[128];
    sprintf(Header, "HTTP/1.0 200 OK\r\n"
                    "Content-type: text/html\r\n"
                   "Content-length: 66\r\n\r\n");
    
    if( write(*sock, Header, strlen(Header)) == -1){
        perror("Something went wrong writing header.");
        exit(-1);
    }
}
