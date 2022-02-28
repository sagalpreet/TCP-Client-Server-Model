#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

#define DEFAULT_PORT 8080
#define DEFAULT_MAX_CONN 100
#define MAX_STRING_LEN 1024
#define TOKEN_LENGTH 64
#define DEBUG 0

// global variables
int SOCKET_FD;
uint NEXT_CLIENT_ID;
pthread_mutex_t TERMINAL_LOG, FILE_LOG, ASSIGN_CLIENT_ID;
FILE *SERVER_RECORDS;

// data structures
typedef struct _node
{
    float val;
    struct _node *next;
} node;

typedef struct
{
    node *head;
    int size;
} stack;

typedef struct
{
    /**
     * @brief structure used to store a token.
     * type  0: integer number
     * type  1: floating point number
     * type  2: operator
     * type -1: invalid
     */
    char val[TOKEN_LENGTH];
    char type;
} token;

// data structure method declarations
void push(stack *s, float val);
float pop(stack *s);
float top(stack *s);
stack *makeStack();
node *makeNode(float val);

// helper function declarations
void serverSetup(int PORT, int MAX_CONN, int ADDR);
void clientConnect(int socketFD, struct sockaddr_in serverAddress, int addrlen);
void evaluatePostfix(char *string);
void *handleConnections(void *arg);
token nextToken(char *string, int *index);
token nextNumber(char *string, int *index);
token nextOperator(char *string, int *index);

// The main function
int main(int argc, char **argv)
{
    NEXT_CLIENT_ID = 0;
    setbuf(stdout, NULL);
    SERVER_RECORDS = fopen("server_records.txt", "a");
    setbuf(SERVER_RECORDS, NULL);

    int PORT = DEFAULT_PORT;
    int MAX_CONN = DEFAULT_MAX_CONN;
    in_addr_t ADDR = INADDR_ANY;

    // decode arguments
    // if port number is specified specifically as
    // a command line argument, update the port
    // value from default value
    if (argc > 1)
        PORT = atoi(argv[1]);
    if (argc > 2)
        MAX_CONN = atoi(argv[2]);
    if (argc > 3)
        ADDR = inet_addr(argv[3]);

    serverSetup(PORT, MAX_CONN, ADDR);

    fclose(SERVER_RECORDS);

    return 0;
}

// data structure method definitions
stack *makeStack()
{
    /**
     * @brief return a pointer
     * to an empty stack
     */

    // allocate memory and initialize an empty stack
    stack *s = (stack *)malloc(sizeof(stack));
    s->head = 0;
    s->size = 0;

    return s;
}

node *makeNode(float val)
{
    /**
     * @brief return a pointer to a
     * node initialized with val.
     * next is set to null.
     */

    // allocate memory and initialize a node
    node *n = (node *)malloc(sizeof(node));
    n->val = val;
    n->next = 0;
}

void push(stack *s, float val)
{
    /**
     * @brief add a new node with val
     * to the top of the stack
     */

    // make a new node with val
    node *n = makeNode(val);

    /**
     * @brief check if stack is empty.
     * If empty: make n as head
     * Else: add to the start
     */
    if (s->head)
    {
        n->next = s->head;
        s->head = n;
    }
    else
    {
        s->head = n;
    }

    // increment the size of stack
    s->size += 1;
}

float pop(stack *s)
{
    /**
     * @brief check if stack is empty
     * else pop the top most element and return it
     */

    if (!s->head)
        return 0;

    node *temp = s->head;
    s->head = temp->next;

    float val = temp->val;

    free(temp);
    (s->size)--;

    return val;
}

float top(stack *s)
{
    /**
     * @brief check if stack is empty
     * else return the value of top most element
     */

    return (s->head)->val;
}

// helper function definitions
void serverSetup(int PORT, int MAX_CONN, int ADDR)
{
    // create a socket
    int socketFD = SOCKET_FD = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFD == -1)
    {
        pthread_mutex_lock(&TERMINAL_LOG);
        fprintf(stderr, "Error: Attempt to create a socket failed...\n");
        pthread_mutex_unlock(&TERMINAL_LOG);
        exit(errno);
    }
    else
    {
        pthread_mutex_lock(&TERMINAL_LOG);
        fprintf(stdout, "Socket Created successfully...\n");
        pthread_mutex_unlock(&TERMINAL_LOG);
    }

    // socket address setup
    struct sockaddr_in serverAddress;
    int addrlen = sizeof(serverAddress);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = ADDR;
    serverAddress.sin_port = htons(PORT);

    // binding address to the socket
    if (bind(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        pthread_mutex_lock(&TERMINAL_LOG);
        fprintf(stderr, "Error: Failed to bind the socket\n");
        pthread_mutex_unlock(&TERMINAL_LOG);
        exit(errno);
    }
    else
    {
        pthread_mutex_lock(&TERMINAL_LOG);
        fprintf(stdout, "Socket Binded successfully...\n");
        pthread_mutex_unlock(&TERMINAL_LOG);
    }

    // convert socket to listening socket
    if (listen(socketFD, MAX_CONN) == -1)
    {
        /**
         * @note MAX_CONN is not necessarily required for
         * our tcp server because tcp server supports
         * retransmission. Effectively, if queue size in
         * our case exceeds MAX_CONN, request will be declined
         * but re-attempted in future
         *
         */
        pthread_mutex_lock(&TERMINAL_LOG);
        fprintf(stderr, "Error: Listen failed on the socket\n");
        pthread_mutex_unlock(&TERMINAL_LOG);
        exit(errno);
    }
    else
    {
        pthread_mutex_lock(&TERMINAL_LOG);
        fprintf(stdout, "Server Listening...\n\n");
        pthread_mutex_unlock(&TERMINAL_LOG);
    }

    clientConnect(socketFD, serverAddress, addrlen);

    close(socketFD);
}

void clientConnect(int socketFD, struct sockaddr_in serverAddress, int addrlen)
{
    while (1)
    {
        // allocate memory to store the file descriptor of
        // peer socket, this will be freed inside the thread
        int *peer_socket = (int *)malloc(sizeof(int));

        pthread_mutex_lock(&TERMINAL_LOG);
        fprintf(stdout, "Waiting for new connection ...\n");
        pthread_mutex_unlock(&TERMINAL_LOG);

        // attempt to accept the connection request
        *peer_socket = accept(socketFD, (struct sockaddr *)&serverAddress, (socklen_t *)&addrlen);

        // handle case if couldn't connect
        if ((*peer_socket) == -1)
        {
            pthread_mutex_lock(&TERMINAL_LOG);
            fprintf(stderr, "Could't connect with the client %d\n", errno);
            pthread_mutex_unlock(&TERMINAL_LOG);

            free(peer_socket);
            continue;
        }
        pthread_mutex_lock(&TERMINAL_LOG);
        fprintf(stdout, "Connection established with socket file descriptor %d\n", *peer_socket);
        pthread_mutex_unlock(&TERMINAL_LOG);

        // connection handling on a different thread
        pthread_t thread;
        pthread_create(&thread, NULL, handleConnections, peer_socket);
    }
}

void evaluatePostfix(char *string)
{
    /**
     * @brief The function evaluates postfix
     * expression given in string
     *
     * After evaluation, the value is written
     * to the string itself
     *
     * @arg Takes the string as argument
     * @return void
     *
     */

    // check if the string is valid
    // i.e. the pointer is valid and not null
    if (!string)
    {
        strcpy(string, "EMPTY EXPRESSION");
        return;
    }

    // get the length of the string
    int length = strlen(string);

    // initialize a stack
    stack *s = makeStack();

    // read through the string
    int index = 0;
    char hasFloat = 0;
    token t;

    while (index < length)
    {
        t = nextToken(string, &index);
        if (t.type == 0 || t.type == 1) // number
        {
            hasFloat |= t.type;
            push(s, atof(t.val));
        }
        else if (t.type == 2) // operator
        {
            char op = t.val[0];
            float a, b;

            if (!s->size)
            {
                strcpy(string, "INVALID EXPRESSION");
                return;
            }
            b = pop(s);

            if (!s->size)
            {
                strcpy(string, "INVALID EXPRESSION");
                return;
            }
            a = pop(s);

            switch (op)
            {
            case '+':
                a += b;
                break;
            case '-':
                a -= b;
                break;
            case '*':
                a *= b;
                break;
            case '/':
                if (b == 0)
                {
                    strcpy(string, "DIVISION BY ZERO");
                    while (s -> size) pop(s);
                    return;
                }
                a /= b;
                break;
            default:
                strcpy(string, "INVALID EXPRESSION");
                while (s -> size) pop(s);
                return;
            }

            push(s, a);
        }
        else
        {
            strcpy(string, "INVALID EXPRESSION");
            return;
        }
    }

    if (s->size == 1)
    {
        float ans = top(s);
        if (ans == (int)ans && hasFloat == 0)
        {
            sprintf(string, "%d", (int)ans);
        }
        else
        {
            sprintf(string, "%f", ans);
        }
    }
    else
        strcpy(string, "INVALID EXPRESSION");
    
    while (s -> size) pop(s);

    free(s);
}

void *handleConnections(void *arg)
{
    /**
     * @brief this function serves one of the clients.
     * It expects input from the client.
     * And the thread dies out if client stops.
     *
     */

    int start_time = time(NULL);

    // assigning a client id to the client
    pthread_mutex_lock(&ASSIGN_CLIENT_ID);
    uint id = NEXT_CLIENT_ID;
    NEXT_CLIENT_ID++;
    pthread_mutex_unlock(&ASSIGN_CLIENT_ID);

    char id_string[1000] = {0};
    sprintf(id_string, "%u", id);

    int peer_socket = *((int *)arg);

    // for information exchange
    char buffer[MAX_STRING_LEN + 1] = {0};

    // sending the client id to client
    if (send(peer_socket, id_string, sizeof(char) * strlen(id_string), 0) == -1)
    {
        pthread_mutex_lock(&TERMINAL_LOG);
        fprintf(stderr, "Error: Couldn't send client id to client %u\n", id);
        pthread_mutex_unlock(&TERMINAL_LOG);
    }

    while (1) // for non-pipelined persistent connection
    {
        // clear buffer
        memset(buffer, 0, MAX_STRING_LEN + 1);

        // read input from client
        int valread = recv(peer_socket, buffer, MAX_STRING_LEN + 1, 0);

        pthread_mutex_lock(&FILE_LOG);
        fprintf(SERVER_RECORDS, "%d %s ", id, buffer);
        pthread_mutex_unlock(&FILE_LOG);

        // if client has shutdown
        if (valread == 0)
        {
            pthread_mutex_lock(&TERMINAL_LOG);
            fprintf(stderr, "Shutting down connection with client %u\n", id);
            pthread_mutex_unlock(&TERMINAL_LOG);
            
            free(arg);
            close(peer_socket);

            return NULL;
        }

        // reverse the string in-place
        evaluatePostfix(buffer);

        pthread_mutex_lock(&FILE_LOG);
        fprintf(SERVER_RECORDS, "%s %ld\n", buffer, time(NULL) - start_time);
        pthread_mutex_unlock(&FILE_LOG);

        // send back the result
        if (send(peer_socket, buffer, sizeof(char) * strlen(buffer), 0) == -1)
        {
            pthread_mutex_lock(&TERMINAL_LOG);
            fprintf(stderr, "Error: Couldn't send result to peer %u\n", id);
            pthread_mutex_unlock(&TERMINAL_LOG);
            free(arg);

            return NULL;
        }
    }

    free(arg);
    close(peer_socket);

    return NULL;
}

token nextToken(char *string, int *index)
{
    /**
     * @brief returns the next token starting from
     * index
     */

    while (string[*index] == ' ')
        (*index)++;

    token t;
    memset(t.val, 0, TOKEN_LENGTH);

    if (string[*index] >= '0' && string[*index] <= '9')
    {
        t = nextNumber(string, index);
    }
    else if (string[*index] == '+' || string[*index] == '-' || string[*index] == '*' || string[*index] == '/')
    {
        t = nextOperator(string, index);
    }
    else
    {
        t.type = -1;
    }

    return t;
}

token nextNumber(char *string, int *index)
{
    /**
     * @brief reads the next number
     * starting from index
     *
     * assumes that string[index] is a
     * numeric character
     */

    token t;
    t.type = 0;
    memset(t.val, 0, TOKEN_LENGTH);

    // i stores the index from which next character is to be read
    int i = (*index);

    // j stores the index at which next character is to be written
    int j = 0;

    // to maintain a check if decimal point has already been encountered
    char gotDecimalPoint = 0;

    // iterate over the string to fetch full number
    while ((string[i] >= '0' && string[i] <= '9') || string[i] == '.')
    {
        if (string[i] == '.')
        {
            if (gotDecimalPoint)
                break;
            else
                gotDecimalPoint = 1;
        }

        t.val[j] = string[i];
        i++;
        j++;
    }

    // if decimal point is encountered, change type to float
    if (gotDecimalPoint)
        t.type = 1;

    // update index to new value
    *index = i;

    return t;
}

token nextOperator(char *string, int *index)
{
    /**
     * @brief reads the next operator
     * starting from index
     *
     * assumes that string[index] is an
     * operator
     */

    token t;
    t.type = 2;
    memset(t.val, 0, TOKEN_LENGTH);

    t.val[0] = string[*index];
    (*index)++;

    return t;
}
