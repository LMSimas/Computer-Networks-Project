#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
 #include <errno.h>

#define BUFFER_SIZE 1024
#define NODELIST_SIZE 16


typedef struct _node {
    char netid [BUFFER_SIZE];
    
    char nodeid [BUFFER_SIZE];
    char nodeIP[BUFFER_SIZE];
    char nodeTCP[BUFFER_SIZE];

    char extern_id [BUFFER_SIZE];
    char extern_IP [BUFFER_SIZE];
    char extern_PORT [BUFFER_SIZE];

    char backup_id [BUFFER_SIZE];
    char backup_IP [BUFFER_SIZE];
    char backup_PORT [BUFFER_SIZE];
} node_s;

typedef struct _clientNode{
    int fd;
    struct _clientNode *next;
}clientNode;

void init_udpsocket(int* sockfd, struct addrinfo *hints, struct addrinfo **server_info, char* argv[]);
void prepare_tcpClient(struct addrinfo *cli_hints, struct addrinfo *cli_res, int* cli_fd);
void prepare_tcpServer(struct addrinfo *ser_hints, struct addrinfo *ser_res, int *ser_listenfd);

void print_TCParray();

bool validate_inputArgs(int argc, char* argv[]);

int choose_extern();

void udp_regRequest(char message[], struct addrinfo * udp_serverInfo, int sockfd);



void rcv_newCLient(struct sockaddr *myclient_addr,int myclient_fd, int ser_listenfd);

/*RCV Messages*/
void rcv_msgFromServer(int cli_fd);
void rcv_msgFromClients(int myclient_fd);
void rcv_OkReg(int sockfd, char message[], struct sockaddr * server_addr, socklen_t * addrlen, struct addrinfo * ser_hints,
struct addrinfo * ser_res, int * ser_listenfd);
void rcv_OkUnreg(int sockfd, char message[], struct sockaddr * server_addr, socklen_t * addrlen);
void rcv_nodeslist(int sockfd, char message[], struct sockaddr * server_addr, socklen_t * addrlen, char message1[], char* token);

/*COMMANDS*/
void notreg_stdinCommands(char buffer[], char command[], char message[], int sockfd, struct addrinfo * udp_serverInfo);
void regwait_stdinCommands(char buffer[], char command[]);
void reg_stdinCommands(char buffer[], char command[], char message[],int sockfd, struct addrinfo * udp_serverInfo, int cli_fd, int ser_listenfd);
void notregwait_stdinCommands(char buffer[], char command[]);
void listwait_stdinCommands(char buffer[], char command[]);

/*MultiClients*/
void insertNode_clientsList(clientNode *new_node);
void free_clientNode(clientNode * node);
clientNode* alloc_clientNode(int client_fd);
int get_ClientsMaxfd();
void prepare_ClientFDSets(fd_set * rfds);
int get_ClientISSET(fd_set * rfds);
 