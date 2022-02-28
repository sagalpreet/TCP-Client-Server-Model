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

#define DEFAULT_PORT 8080
#define DEFAULT_MAX_CONN 100
#define MAX_STRING_LEN 1024
#define TOKEN_LENGTH 64
#define DEBUG 0

// global variables
int SOCKET_FD;

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
void evaluatePostfix(char *string);
void *handleConnections(void *arg);
token nextToken(char *string, int *index);
token nextNumber(char *string, int *index);
token nextOperator(char *string, int *index);

// The main function
int main(int argc, char **argv)
{

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

    // create a socket
    int socketFD = SOCKET_FD = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFD == -1)
    {
        fprintf(stderr, "Error: Attempt to create a socket failed...\n");
        exit(errno);
    }
    else
    {
        fprintf(stdout, "Socket Created successfully...\n");
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
        fprintf(stderr, "Error: Failed to bind the socket\n");
        exit(errno);
    }
    else
    {
        fprintf(stdout, "Socket Binded successfully...\n");
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
        fprintf(stderr, "Error: Listen failed on the socket\n");
        exit(errno);
    }
    else
    {
        fprintf(stdout, "Server Listening...\n\n");
    }

    while (1)
    {
        // allocate memory to store the file descriptor of
        // peer socket, this will be freed inside the thread
        int *peer_socket = (int *)malloc(sizeof(int));

        fprintf(stdout, "Waiting for new connection ...\n");

        // attempt to accept the connection request
        *peer_socket = accept(socketFD, (struct sockaddr *)&serverAddress, (socklen_t *)&addrlen);

        // handle case if couldn't connect
        if ((*peer_socket) == -1)
        {
            fprintf(stderr, "Could't connect with the client %d\n", errno);

            free(peer_socket);
            continue;
        }
        fprintf(stdout, "Connection established with socket file descriptor %d\n", *peer_socket);

        // connection handling on a different thread
        pthread_t thread;
        pthread_create(&thread, NULL, handleConnections, peer_socket);
    }

    close(socketFD);

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
                    return;
                }
                a /= b;
                break;
            default:
                strcpy(string, "INVALID EXPRESSION");
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
    int peer_socket = *((int *)arg);
    char buffer[MAX_STRING_LEN + 1] = {0};

    int valread = recv(peer_socket, buffer, MAX_STRING_LEN + 1, 0);

    // if error encountered while reading request
    if (valread == -1)
    {
        fprintf(stderr, "Error: Couldn't read request from peer %d\n", peer_socket);
        free(arg);

        return NULL;
    }

    // reverse the string in-place
    evaluatePostfix(buffer);

    // send back the result
    if (send(peer_socket, buffer, sizeof(char) * strlen(buffer), 0) == -1)
    {
        fprintf(stderr, "Error: Couldn't send result to peer %d\n", peer_socket);
        free(arg);

        return NULL;
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