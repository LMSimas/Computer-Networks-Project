#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define NODELIST_SIZE 16
#define TABLE_SIZE 50


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
    char ip[BUFFER_SIZE];
    char tcp[BUFFER_SIZE];
    char clientBuffer[BUFFER_SIZE];
    struct _clientNode *next;
}clientNode;

void init_udpsocket(int* sockfd, struct addrinfo *hints, struct addrinfo **server_info, char* argv[]);
void prepare_tcpClient(struct addrinfo *cli_hints, struct addrinfo *cli_res, int* cli_fd);
void prepare_tcpServer(struct addrinfo *ser_hints, struct addrinfo *ser_res, int *ser_listenfd);

void print_TCParray();

bool validate_inputArgs(int argc, char* argv[]);

int choose_extern();

void udp_regRequest(char message[], struct addrinfo * udp_serverInfo, int sockfd);



void rcv_newCLient(struct sockaddr *myclient_addr,int myclient_fd, int ser_listenfd, int* cli_fd, struct addrinfo *cli_hints, struct addrinfo *cli_res);

/*RCV Messages*/
void rcv_msgFromServer(int* cli_fd, struct addrinfo *cli_hints, struct addrinfo *cli_res);
void rcv_msgFromClients(int myclient_fd, int cli_fd);
void rcv_OkReg(int sockfd, char message[], struct sockaddr * server_addr, socklen_t * addrlen, struct addrinfo * ser_hints,
struct addrinfo * ser_res, int * ser_listenfd);
void rcv_OkUnreg(int sockfd, char message[], struct sockaddr * server_addr, socklen_t * addrlen);
void rcv_nodeslist(int sockfd, char message[], struct sockaddr * server_addr, socklen_t * addrlen, char message1[], char* token);

/*COMMANDS*/
void notreg_stdinCommands(char buffer[], char command[], char message[], int sockfd, struct addrinfo * udp_serverInfo, struct addrinfo *cli_hints, struct addrinfo *cli_res, int *cli_fd);
void regwait_stdinCommands(char buffer[], char command[]);
void reg_stdinCommands(char buffer[], char command[], char message[],int sockfd, struct addrinfo * udp_serverInfo, int cli_fd, int ser_listenfd);
void notregwait_stdinCommands(char buffer[], char command[]);
void listwait_stdinCommands(char buffer[], char command[]);

/*MultiClients*/
void insertNode_clientsList(clientNode *new_node);
void free_clientNode(clientNode * node);
clientNode* alloc_clientNode(int client_fd, char * ip, char * tcp);
int get_ClientsMaxfd();
void prepare_ClientFDSets(fd_set * rfds);
int get_ClientISSET(fd_set * rfds);

/*Spread received message to clients*/
void send_ToMyClients(int adver_id, int font_fd, int op);
void closenFree_clientsList();
int get_idFromExpTable(int my_client_fd);

void advertise_myExternalNode(int advertise_id, int cli_fd);

void extern_randomClient();
void compare_externBackupNodes(int left_nodeID);
void clean_expTable(int left_fd);
void clean_expTable2(int left_fd, int cli_fd);
void rmvNode_clientsList(int left_node);
void rmvNode_clientsList(int myclient_fd);