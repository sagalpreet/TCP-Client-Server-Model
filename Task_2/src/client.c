#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

#define DEFAULT_INTERFACE "127.0.0.1"
#define DEFAULT_PORT 8080
#define MAX_STRING_LEN 1024

#define max(a, b) (a > b) ? a:b

#define SUPPORTS_MULTI_LINE 0

// global variables
int SOCKET_FD;

// function declarations
void interact(int socketFD);

// The main function
int main(int argc, char **argv) {
    /**
     * @brief no buffer for stdout
     * so no need to flush.
     * This helps in interactive
     * experience.
     * 
     */
    setbuf(stdout, NULL);


    int PORT = DEFAULT_PORT;
    in_addr_t INTERFACE = inet_addr(DEFAULT_INTERFACE);

    // decode arguments
    // if port number is specified specifically as 
    // a command line argument, update the port
    // value from default value
    if (argc > 1) PORT = atoi(argv[1]);
    if (argc > 2) INTERFACE = inet_addr(argv[2]);

    // create a socket
    int socketFD = SOCKET_FD = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFD == -1) {
        fprintf(stderr, "Error: Attempt to create a socket failed...\n");
        exit(errno);
    } else {
        fprintf(stdout, "Socket %d Created successfully...\n", socketFD);
    }

    // socket address setup
    struct sockaddr_in serverAddress;
    int addrlen = sizeof(serverAddress);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INTERFACE;
    serverAddress.sin_port = htons(PORT);

    // binding address to the socket
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        fprintf(stderr, "Error: Failed to connect to server\n");
        exit(errno);
    } else {
        fprintf(stdout, "Socket Binded successfully...\n\n");
    }

    // client id assigned by server
    char id[1000] = {0};

    if (read(socketFD, id, MAX_STRING_LEN) == -1)
    {
        fprintf(stdout, "Could not read the client id\n");
    }
    else
    {
        fprintf(stdout, "Client ID: %s\n", id);
    }

    interact(socketFD); // talk to server
}

// function definitions
void interact(int socketFD) {
    char buffer[MAX_STRING_LEN + 1] = {0};
    char c;

    while (1)
    {
        fprintf(stdout, "Enter the string: ");

        // read string from stdin
        // \n denotes end of string
        for (int i = 0; i <= MAX_STRING_LEN; i++) {
            c = getchar();

            int isEnd = (c == EOF);
            if (!SUPPORTS_MULTI_LINE) isEnd |= (c == '\n');

            if (isEnd) {
                buffer[i] = 0;
                break;
            }
            buffer[i] = c;
        }

        // send the string to server
        if (send(socketFD, buffer, max(1, sizeof(char) * strlen(buffer)), 0) == -1) {
            fprintf(stderr, "Error: sending input %d\n", errno);
        }

        // clear buffer for reading output
        memset(buffer, 0, MAX_STRING_LEN);

        // read output from server
        if (read(socketFD, buffer, MAX_STRING_LEN) == -1) {
            fprintf(stderr, "Error: fetching output %d\n", errno);
        }

        // for printing output to next line
        if (c == EOF) fprintf(stdout, "\n");

        fprintf(stdout, "Output from Server: %s\n\n", buffer);

        if (c == EOF) {
            clearerr(stdin);
        }
    }

    close(socketFD);
}