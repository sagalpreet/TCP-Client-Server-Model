TCP CLIENT SERVER MODEL

---------------

1. Desription

Task-1:

Client sends the string to the server and server reverses the string sent by the client and sends it back to the client.



Task-2:

The client connects to the server, and then asks the user for input. The user enters a simple arithmetic expression string in postfix form (e.g., "1 2 +", "5 6 22.3 * +"). The user's input is sent to the server via the connected socket.

The server reads the user's input from the client socket, evaluates the postfix expression, and sends the result back to the client as well as writes the following in a file named “server_records.txt”. [at the beginning create an empty file] <client_id> <query> <answer> <time_elapsed>

---------------

2. File Structure

.
├── README.txt
├── Task_1
│   ├── client.c
│   └── server.c
└── Task_2
    ├── client.c
    └── server.c
    
---------------

1. Compiling the code:
> gcc server.c -o server -pthread
> gcc client.c -o client

---------------

2. Running the server:
> ./server

- Optional arguments can be given PORT, MAX_CONN, ADDRESS
> ./server 9999
> ./server 9999 1
> ./server 9999 1 127.0.0.1

- By default:
PORT = 8080
MAX_CONN = 100
ADDRESS = INADDR_ANY (all available interfaces)

---------------

3. Running the client:
> ./client

- Optional arguments can be given PORT, ADDRESS
> ./client 9999
> ./client 9999 127.0.0.1

- By default:
PORT = 8080
ADDRESS = 127.0.0.1

---------------

4. Important Points

- TASK 2
---- To give input from a file instead of terminal, change client to non-interactive mode by changing define INTERACTIVE 1 to 0.
---- The maximum length allowed for the message can be changed similarly by altering the value defined.
---- Ctrl+d or return denotes end of one input.
---- Ctrl+c to kill the client or server.
