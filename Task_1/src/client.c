#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <string.h>

#define DEFAULT_INTERFACE "127.0.0.1"
#define DEFAULT_PORT 8081
#define MAX_STRING_LEN 1024

// global variables
int SOCKET_FD;

// function declarations
void interact(int socketFD);
void closeSocket();

// The main function
int main(int argc, char **argv) {
    atexit(closeSocket); // close socket on exit

    int PORT = DEFAULT_PORT;
    char INTERFACE[100] = DEFAULT_INTERFACE;

    // decode arguments
    // if port number is specified specifically as 
    // a command line argument, update the port
    // value from default value
    if (argc > 1) PORT = atoi(argv[1]);
    if (argc > 2) strcpy(INTERFACE, argv[2]);

    // create a socket
    int socketFD = SOCKET_FD = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFD == -1) {
        fprintf(stderr, "Error: Attempt to create a socket failed...\n");
        exit(errno);
    } else {
        fprintf(stdout, "Socket Created successfully...\n");
    }

    // socket address setup
    struct sockaddr_in serverAddress;
    int addrlen = sizeof(serverAddress);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(INTERFACE);
    serverAddress.sin_port = htons(PORT);

    // binding address to the socket
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        fprintf(stderr, "Error: Failed to connect to server\n");
        exit(errno);
    } else {
        fprintf(stdout, "Socket Binded successfully...\n");
    }

    interact(socketFD); // talk to server
}

// function definitions
void interact(int socketFD) {
    char buffer[MAX_STRING_LEN + 1] = {0};
    char c;
    while (1) {
        fprintf(stdout, "Enter the string: ");

        // read string from stdin
        // \n denotes end of string
        for (int i = 0; i <= MAX_STRING_LEN; i++) {
            c = getchar();
            if (c == '\n') break;
            buffer[i] = c;
        }

        // send the string to server
        if (send(socketFD, buffer, sizeof(char) * strlen(buffer), 0) == -1) {
            fprintf(stderr, "Error: sending input %d\n", errno);
            continue;
        }

        // clear buffer for reading output
        memset(buffer, 0, MAX_STRING_LEN);

        // read output from server
        if (read(socketFD, buffer, MAX_STRING_LEN) == -1) {
            fprintf(stderr, "Error: fetching output %d\n", errno);
            continue;
        }

        fprintf(stdout, "Output from Server: %s\n", buffer);
    }
}

void closeSocket() {
    if (SOCKET_FD != -1)
        close(SOCKET_FD);
}