#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <string.h>

#define DEFAULT_PORT 8080
#define DEFAULT_MAX_CONN 100
#define MAX_STRING_LEN 1024

// global variables
int SOCKET_FD;


// The following code contains function declarations
void reverseString(char *string);
void* handleConnections(void *arg);
void closeSocket();


// The main function
int main(int argc, char **argv) {
    atexit(closeSocket); // close socket on exit

    int PORT = DEFAULT_PORT;
    int MAX_CONN = DEFAULT_MAX_CONN;

    // decode arguments
    // if port number is specified specifically as 
    // a command line argument, update the port
    // value from default value
    if (argc > 1) PORT = atoi(argv[1]);
    if (argc > 2) MAX_CONN = atoi(argv[2]);

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
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    // binding address to the socket
    if (bind(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        fprintf(stderr, "Error: Failed to bind the socket\n");
        exit(errno);
    } else {
        fprintf(stdout, "Socket Binded successfully...\n");
    }

    // convert socket to listening socket
    if (listen(socketFD, MAX_CONN) == -1) {
        /**
         * @note MAX_CONN is not necessarily required for
         * our tcp server because tcp server supports
         * retransmission. Effectively, if queue size in
         * our case exceeds MAX_CONN, request will be declined
         * but re-attempted in future
         * 
         */
        fprintf(stderr, "Error: Listen failed on the socket\n");
        exit(errno);
    } else {
        fprintf(stdout, "Server Listening...\n\n");
    }

    while (1) {
        // allocate memory to store the file descriptor of
        // peer socket, this will be freed inside the thread
        int *peer_socket = (int *) malloc(sizeof(int));

        fprintf(stdout, "Waiting for new connection ...\n");

        // attempt to accept the connection request
        *peer_socket = accept(socketFD, (struct sockaddr *)&serverAddress, (socklen_t *)&addrlen); // try to connect to a client

        // handle case if couldn't connect due to connection limit reached
        if ((*peer_socket) == -1)
        {
            fprintf(stderr, "Connection limit reached, could't connect\n");

            free(peer_socket);
            continue;
        }
        
        fprintf(stdout, "Connection established with socket file descriptor %d\n", *peer_socket);

        // connection handling on a different thread
        pthread_t thread;
        pthread_create(&thread, NULL, handleConnections, peer_socket);
    }

    
    return 0;
}


// function definitions
void reverseString(char *string) {
    /**
     * @brief The function reverses the string
     * in place
     * 
     * @arg Takes the string as argument
     * @return void
     * 
     */

    // check if the string is valid
    // i.e. the pointer is valid and not null
    if (!string) return;

    // get the length of the string
    int length = strlen(string);

    /**
     * iterate till the mid
     * of the string
     * swap characters
     * equidistant from start and end
     */
    for (int i = 0; i < length/2; i++) {
        char temp = string[length - i - 1];
        string[i] = string[length - i - 1];
        string[length - i - 1] = temp;
    }

}

void* handleConnections(void *arg)
{
    int peer_socket = *((int*) arg);
    char buffer[MAX_STRING_LEN + 1] = {0};
    int valread = recv(peer_socket, buffer, MAX_STRING_LEN + 1, 0);

    // if error encountered while reading request
    if (valread == -1) {
        fprintf(stderr, "Error: Couldn't read request from peer %d\n", peer_socket);
        free(arg);
        send(peer_socket, &errno, sizeof(errno), 0);

        return NULL;
    }

    reverseString(buffer); // reverse the string in-place

    send(peer_socket, buffer, sizeof(char)*strlen(buffer), 0); // send the client confirmation/disapproval depending upon whether the request is queued or not
    free(arg);

    return NULL;
}

void closeSocket() {
    if (SOCKET_FD != -1)
        close(SOCKET_FD);
}