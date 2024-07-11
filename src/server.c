#include "mime.h"
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERROR_FILE	0
#define REG_FILE	1
#define DIRECTORY	2
#define FILESIZEMAX	200000000
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 +  Function: typeOfFile()
 +  Description: Tells us if the request is a environment or a regular file
 +
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int typeOfFile(char *filepath)
{
	struct stat buf; /* to stat the file/dir */

	if (stat(filepath, &buf) != 0) {
		perror("Failed to identify specified file");
		fprintf(stderr, "[ERROR] stat() on file: |%s|\n", filepath);
		fflush(stderr);
		return (ERROR_FILE);
	}

	printf("file path: %s, filesize: %li bytes\n", filepath, buf.st_size);

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
// TODO: Add to general http parser
void extractFileRequest(char *method, char *req, char *buff)
{
	char  *p	  = buff;
	char  *end	  = strchr(p, '/'); // find the first slash
	size_t method_len = end - p;
	strncpy(method, p, method_len - 1);
	method[method_len] = '\0';

	// Find the name of the requested file.
	char *start    = end; // take the start of the request to be the start
	end	       = strchr(start, ' '); // find the terminating space
	size_t req_len = end - start;
	strncpy(req, start, req_len);
	req[req_len] = '\0';
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 + Open a file and handle size calculations.
 +
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// TODO: Somehow integrate with cacheing??
void handleOpenFile(const char *filepath, int *fileHandle, off_t *file_size,
		    int *fileExist)
{
	if ((*fileHandle = open(filepath, O_RDONLY)) == -1) {
		fprintf(stderr, "File does not exist: %s\n", filepath);
		*fileExist = 0;
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
void sendFile(int *sock, char *buffer, int *fd, off_t *file_size)
{
	ssize_t s;
	int	sendbuffer = 0;
	memset(buffer, 0, sizeof(*buffer));

	while (sendbuffer <= *file_size) {
		sendbuffer = sendbuffer + 1024;
		// read file
		if ((s = read(*fd, buffer, 1024)) == -1) {
			perror("Something went wrong reading to buffer.");
			exit(-1);
		}
		// send file
		if ((write(*sock, buffer, s)) == -1) {
			perror("Something went wrong writing file.");
			exit(-1);
		}
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 + Handle various erroneous http requests.
 +
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void foured(int id, char *webDir, int *sock, char *buff)
{
	int   fileHandle;
	int   fileExist = 1;
	off_t file_size;
	char  filepath[10];
	char  Header[1024];
	sprintf(filepath, "%s/a/%d.jpg", webDir, id);
	handleOpenFile(filepath, &fileHandle, &file_size, &fileExist);
	char *mime = mime_type_get(filepath);

	// TODO: can make similar hashmap for this as to mime types.
	char *idnames[] = { "Bad Request", "Forbidden", "Not Found",
			    "Method not allowed"
			    "Internal server error" };
	int   ids[]	= { 400, 403, 404, 405, 500 };
	int   size	= sizeof(ids) / sizeof(ids[0]);
	int   i		= 0;
	for (; i < size; i++) {
		if (ids[i] == id) {
			break; // Return the index of the matching element
		}
	}

	sprintf(Header,
		"HTTP/1.1 %d %s\r\n"
		"Content-type: %s\r\n"
		"Content-length: %ld\r\n\r\n",
		id, idnames[i], mime, file_size);
	// Send header.
	if (write(*sock, Header, strlen(Header)) == -1) {
		perror("Something went wrong writing header.");
		exit(-1);
	}
	sendFile(sock, buff, &fileHandle, &file_size);
	close(fileHandle);
	printf("\nResponse:\n%s\n", Header);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 + Generate a random string for a file.
 +      - Don't forget to free() the generated string.
 +
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char *randString(char *s, int len)
{
	const char *chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	int i, n;

	srand(time(NULL));
	for (i = 0; i < len; i++) {
		n    = rand() % (int)(strlen(chars) - 1);
		s[i] = chars[n];
	}
	s[len] = '\0';

	return s;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 +  Function: handleGET()
 +  Description: Send the requested file.
 +
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void handleGET(char *fileToSend, int sock, char *webDir, char *buff)
{
	char  filepath[256];
	char  Header[1024];
	char  buffer[1024];
	off_t file_size;
	int   fileHandle;
	int   fileType;
	char *mime;

	printf("File Requested: |%s|\n", fileToSend);
	// Build the full path to the file
	sprintf(filepath, "%s%s", webDir, fileToSend);

	// If the requested file is a environment (directory), append
	// 'index.html' file to the end of the filepath
	fileType = typeOfFile(filepath);
	if (strcmp(fileToSend, "/") == 0) {
		sprintf(filepath, "%s%s", filepath, "index.html");
	} else if (fileType == DIRECTORY) {
		foured(400, webDir, &sock, buff);
		return;
	} else {
		// Do nothing.
	}

	// TODO: add to generalised 'foured' aka send stuff to client function
	int fileExist = 1;
	handleOpenFile(filepath, &fileHandle, &file_size, &fileExist);
	if (fileExist == 1) {
		mime = mime_type_get(filepath);
		sprintf(Header,
			"HTTP/1.0 200 OK\r\n"
			"Content-Type: %s\r\n"
			"Content-length: %ld\r\n\r\n",
			mime, file_size);
		printf("\nServer response:\n%s\nfilename: %s\n", Header,
		       filepath);

		if (write(sock, Header, strlen(Header)) == -1) {
			perror("Something went wrong writing header.");
			exit(-1);
		}
		sendFile(&sock, buffer, &fileHandle, &file_size);
		close(fileHandle);
	} else {
		foured(404, webDir, &sock, buff);
	}
}
// Function to return a substring given a start and end substrings. Remember to
// free() the returned value.
char *startend(char *data, char *start, char *end)
{
	char  *Start  = strstr(data, start) + 10;
	char  *End    = strstr(Start, end);
	size_t Length = End ? End - Start : 0;
	char  *name   = (char *)malloc(Length + 2);
	if (!name)
		return NULL;
	strncpy(name, Start, Length);
	name[Length] = '\0';
	return name;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 + Handle a Post request and save the file.
 +
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void handlePOST(char *buffer, int *sock, char *web_dir)
{
	// TODO: add to http parser
	// Parse the body of the request.
	char *start = strstr(buffer, "\r\n\r\n") + 4;
	char *form  = malloc(strlen(start) + 1);
	memmove(form, start, strlen(start));
	form[strlen(start)] = '\0';

	start	   = strstr(form, "\r\n\r\n") + 4;
	char *data = malloc(strlen(start) + 1);
	memmove(data, start, strlen(start));
	data[strlen(start)] = '\0';
	free(form);

	// Parse the Content-Length header to determine the size of the request
	// body.
	char *content_length_str = strstr(buffer, "Content-Length: ") + 16;
	long  content_length	 = atoi(strstr(content_length_str, "\r\n") ?
						content_length_str :
						strstr(content_length_str, "\n"));

	// Check if file is within file size limit
	if (content_length >= FILESIZEMAX) {
		foured(403, web_dir, sock, buffer);
		exit(0);
	}

	// TODO: Repeats the last 6 letters of the filename as an 'extension',
	// probably why the 303 is broken atm too Create a random file name for
	// the uploaded file in the data directory.
	char *filename = startend(data, "filename=\"", "\"");
	char *ext      = strrchr(filename, '.');
	if (ext == NULL) {
		ext = ".txt";
	}
	free(filename);

	char  filepath[2048];
	char *s = malloc(17);
	int   fileExist;
	while (1) {
		snprintf(filepath, 2048, "%s/data/%s%s", web_dir,
			 randString(s, 16), ext);

		// Check if requested filename exists.
		int fd;
		if ((fd = open(filepath, O_RDONLY)) == -1) {
			fileExist = 0;
			break;
			close(fd);
		} else {
			fileExist = 1;
			close(fd);
		}
	}

	// Open the file to write to.
	int file;
	if ((file = open(filepath, O_RDWR | O_CREAT, 0644)) == -1) {
		perror("Error initialising file to write to.");
		foured(500, web_dir, sock, buffer);
		exit(-1);
	}
	printf("\nnew filepath: %s, filesize: %ld bytes\n", filepath,
	       content_length);

	// TODO: Currently doesn't work
	// Write the request body from the buffer.
	if (write(file, data, sizeof(data)) == -1) {
		perror("Something went wrong writing initial buffer to file.");
		foured(500, web_dir, sock, buffer);
		exit(-1);
	}
	free(data);

	// Write any more data that isn't in the buffer.
	sendFile(&file, buffer, sock, &content_length);

	// TODO: currently doesn't work
	// Remove multipart section end
	char *boundary	 = startend(buffer, "boundary=", "\n");
	int   sectionEnd = sizeof(boundary) + 2;
	lseek(file, content_length - sectionEnd, SEEK_SET);
	if ((ftruncate(file, content_length - sectionEnd)) == -1) {
		perror("Failed to truncate end section boundary");
		foured(500, web_dir, sock, buffer);
		exit(-1);
	}
	free(boundary);
	lseek(file, (off_t)0, SEEK_SET);

	// To prevent content-length spoofing check written file size
	off_t fileSize;
	handleOpenFile(filepath, &file, &fileSize, &fileExist);
	if (fileSize >= FILESIZEMAX) {
		foured(403, web_dir, sock, buffer);
		exit(0);
	}
	close(file);

	// TODO: generalised foured function
	// Send back a header.
	char Header[128];
	sprintf(Header,
		"HTTP/1.0 303 See Other\r\n"
		"Location: %s\r\n\r\n",
		strstr(filepath, web_dir) + strlen(web_dir));

	if (write(*sock, Header, strlen(Header)) == -1) {
		perror("Something went wrong writing header.");
		exit(-1);
	}

	printf("\nServer response:\n%s\n", Header);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 +  Function: main()
 +  Description: Program entry point.
 +
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int main(int argc, char **argv)
{
	pid_t pid;
	int   sockFD;

	int  newSockFD;
	int  port;
	char webDir[128];

	socklen_t clientAddressLength;

	// Struct used for bind and accept.
	struct sockaddr_in serverAddress, clientAddress;

	char fileRequest[256];

	// Check for correct program inputs.
	if (argc != 3) {
		fprintf(stderr, "USAGE: %s <port number> <website directory>\n",
			argv[0]);
		exit(-1);
	}

	port = atoi(argv[1]);
	strcpy(webDir, argv[2]);

	if ((pid = fork()) < 0) {
		perror("Cannot fork (for deamon)");
		exit(0);
	} else if (pid != 0) {
		// I am the parent
		printf("Parent pid: %d", pid);
		exit(0);
	}

	// Create our socket, bind it, listen.

	// Create
	if ((sockFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("something went wrong at creating our socket.");
		exit(0);
	}
	// Bind
	serverAddress.sin_family      = AF_INET; /* Internet domain */
	serverAddress.sin_addr.s_addr = INADDR_ANY; /* local machine IP address
						     */
	serverAddress.sin_port = htons(port);
	if ((bind(sockFD, (struct sockaddr *)&serverAddress,
		  sizeof(serverAddress))) == -1) {
		perror("something went wrong at binding socket.");
		exit(0);
	}
	// Listen
	int backlog = 5;
	if ((listen(sockFD, backlog)) == -1) {
		perror("something went wrong at listening.");
		exit(0);
	}

	// Prevent zombie processes
	signal(SIGCHLD, SIG_IGN);

	clientAddressLength = sizeof(clientAddress);

	/*
	 * - accept a new connection and fork.
	 * - If you are the child process,  process the request and exit.
	 * - If you are the parent close the socket and come back to
	 *   accept another connection
	 */

	while (1) {
		// Accept new connection
		if ((newSockFD = accept(sockFD,
					(struct sockaddr *)&clientAddress,
					&clientAddressLength)) == -1) {
			perror("something went wrong at accepting a connection");
			exit(0);
		}

		// Forking off a new process to handle a new connection.
		if ((pid = fork()) < 0) {
			perror("Failed to fork a child process");
			exit(0);
		} else if (pid == 0) {
			// I am the child
			char buff[1024];
			char ref[1024];
			char method[10];

			close(sockFD);
			memset(buff, 0, 1024);
			memset(ref, 0, 1024);

			// Read client request into buff.
			if (read(newSockFD, ref, 1024) == -1) {
				perror("something went wrong reading client request");
			}
			memmove(buff, ref, 1024);

			printf("client request:\n %s\n", ref);

			mime_type_hash_init();
			// TODO: http parser
			// Extract user requested file name.
			extractFileRequest(method, fileRequest, buff);
			if (strcmp(method, "GET") == 0) {
				handleGET(fileRequest, newSockFD, webDir, buff);
			} else if (strcmp(method, "POST") == 0) {
				handlePOST(buff, &newSockFD, webDir);
			} else {
				foured(405, webDir, &newSockFD, buff);
			}

			shutdown(newSockFD, 1);
			close(newSockFD);
			fflush(stdout);
			exit(0);
		} // pid = 0
		else {
			// Parent handed off this connection to its child.
			close(newSockFD);
		}
	}
}
