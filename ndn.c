#include "headerFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*******VARIÁVEIS GLOBAIS*******/
enum {notreg, goingout, regwait, reg, notregwait, listwait} state;

node_s node;
int flag_list = 0;
char TCP_IParray[NODELIST_SIZE][BUFFER_SIZE];//5 for now
char TCP_PORTarray[NODELIST_SIZE][BUFFER_SIZE];
int TCP_NodesCounter = 0;

char tcp_msg[BUFFER_SIZE];

clientNode *clientsList_head = NULL;
int clients_OnLine = 0; //number of clients On server's Line

int main (int argc, char* argv[]){
    char buffer[BUFFER_SIZE], netid[BUFFER_SIZE], nodeid[BUFFER_SIZE], message1[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char *token;

    fd_set rfds;
    int maxfd, counter, sockfd;
    int target_fd;

    /*UDP SERVER VARIABLES*/
    struct addrinfo udp_hints, *udp_serverInfo; //for udp server comms
    int fd,errcode;                      //for udp server comms
    ssize_t n;
    struct sockaddr server_addr;
    socklen_t addrlen;

    /*TCP CLIENT VARIABLES*/
    struct addrinfo cli_hints;
    struct addrinfo *cli_res;
    int cli_fd;

    /*TCP SERVER VARIABLES*/
    struct addrinfo ser_hints;
    struct addrinfo *ser_res;
    int ser_listenfd;
    int ser_acceptfd;
    int myclient_fd;
    struct sockaddr myclient_addr;

    /*VARIAVEIS DESCARTÁVEIS*/
    char return_message[BUFFER_SIZE];

    /*****************************/
    /*****PROGRAM STARTS HERE*****/
    /*****************************/

    /*VALIDATION STEP*/
    if(!validate_inputArgs(argc, argv)) // if NOT validated
        exit(1);//in order 2 exit the programm

    /*APPLICATION STARTS HERE*/
    strcpy(node.nodeIP, argv[1]);
    strcpy(node.nodeTCP, argv[2]);

    init_udpsocket(&sockfd, &udp_hints, &udp_serverInfo, argv);


    /*******MAQUINA DE ESTADOS*******/
    printf("ndn> "); fflush(stdout); //prompt
    state = notreg;
    while(state!=goingout)
    {
        FD_ZERO(&rfds);
        switch (state) //numero de descritores a monitorizar depende do estado
        {
            case notreg:     FD_SET(0, &rfds); maxfd=0; break;
            case regwait:    FD_SET(0, &rfds); FD_SET(sockfd, &rfds); maxfd=sockfd; break;

            case reg:        FD_SET(0, &rfds); FD_SET(cli_fd, &rfds); FD_SET(ser_listenfd, &rfds);//prepare server, listen & stdin FD
            maxfd=ser_listenfd;
            if(clientsList_head!=NULL){//if CLIENTS CONNECTED -- update maxfd and prepare clients FDSets
                prepare_ClientFDSets(&rfds);
                maxfd = get_ClientsMaxfd();//warning prof
            }
             break;//case reg

            case notregwait: FD_SET(0, &rfds); FD_SET(sockfd, &rfds); maxfd=sockfd; break;
            case listwait:   FD_SET(0, &rfds); FD_SET(sockfd, &rfds); maxfd=sockfd; break;
        }//switch(state)

        if (state == notreg && flag_list) //tem que percorrer o notreg (pelo menos) uma vez sem qq input de descritores quando ja tem a lsita de noz
        {
            counter = 1;
        }
        else 
        {
            counter = select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
        }

        if(counter<=0)
        {
            printf("Error: Unexpected (select)\n");
            exit(1);
        }

        for (; counter ; --counter) //corre tantas vezes quantas o retorno do select
            switch (state)
            {

            case notreg:     //---------------------------------------------------------------------------
                if(FD_ISSET(0,&rfds) && !flag_list) //stdin
                {
                    FD_CLR(0,&rfds);
                    notreg_stdinCommands(buffer, command, message, sockfd, udp_serverInfo);
                }//stdin

                if (flag_list) {     //segunda vez que entra neste estado ja c a lista
                    prepare_tcpClient(&cli_hints, cli_res, &cli_fd);
                    udp_regRequest(message, udp_serverInfo, sockfd);           
                }//else if flaglist

            break;//notreg


            case regwait:    //---------------------------------------------------------------------------
                if(FD_ISSET(0,&rfds)) //from stdin
                {
                    FD_CLR(0,&rfds);
                    regwait_stdinCommands(buffer, command);
                }//stdin
                else if (FD_ISSET(sockfd,&rfds)) //from udp server
                {
                    FD_CLR(sockfd,&rfds);
                    rcv_OkReg(sockfd, message, &server_addr, &addrlen, &ser_hints, ser_res, &ser_listenfd);
                }//UDP server socket
            break;//regwait
            
            case reg:        //---------------------------------------------------------------------------
                if(FD_ISSET(0,&rfds)) //stdin
                {
                    FD_CLR(0,&rfds);
                    reg_stdinCommands(buffer, command, message, sockfd, udp_serverInfo);
                }//stdin

                /*******SERVER recebe NEW CLIENT******/
                else if (FD_ISSET(ser_listenfd,&rfds)){ //if a new client has connected
                    FD_CLR(ser_listenfd,&rfds);

                    rcv_newCLient(&myclient_addr, myclient_fd, ser_listenfd);
                }
                /*******CLIENT a receber do SERVER*******/
                else if (FD_ISSET(cli_fd,&rfds)){ 
                    FD_CLR(cli_fd,&rfds);

                    rcv_msgFromServer(cli_fd,return_message);
                }
                
                /*else if(FD_ISSET(myclient_fd, &rfds)){//FZR FOR PARA O VECTOR DE CLIENTS
                    FD_CLR(myclient_fd, &rfds);
                    rcv_msgFromClients(myclient_fd, return_message);
                }*/
                /**********Server recebe dos Client*******/
                else{
                    target_fd = get_ClientISSET(&rfds);
                    if(target_fd != -1){//if we have some fd ISSET
                        rcv_msgFromClients(target_fd, return_message);
                    }

                }
            break;//reg

            case notregwait: //---------------------------------------------------------------------------
                if(FD_ISSET(0,&rfds)) //stdin
                {
                    FD_CLR(0,&rfds);
                    notregwait_stdinCommands(buffer, command);
                }//stdin
                else if (FD_ISSET(sockfd, &rfds)) //udp server
                {
                    FD_CLR(sockfd,&rfds);
                    rcv_OkUnreg(sockfd, message, &server_addr, &addrlen);
                }//UDP socket
            break;//unregwait

            case listwait:    //---------------------------------------------------------------------------
                if(FD_ISSET(0,&rfds)) //stdin
                {
                    FD_CLR(0,&rfds);
                    listwait_stdinCommands(buffer, command);
                }//stdin
                else if (FD_ISSET(sockfd,&rfds)) 
                {
                    FD_CLR(sockfd,&rfds);
                    rcv_nodeslist(sockfd, message, &server_addr, &addrlen, message1, token);
                }//UDP socket
            break;//listwait

            }//switch(state)
        
    }//while()

    close(sockfd);
    freeaddrinfo(udp_serverInfo);

    exit(0);
}//main

void init_udpsocket(int* sockfd, struct addrinfo *udp_hints, struct addrinfo **udp_serverInfo, char* argv[]){
    int errcode;

    *sockfd=socket(AF_INET,SOCK_DGRAM,0);//UDP socket 

    if(*sockfd==-1)
    {
        printf("Error: Unexpected socket\n");
        exit(1);
    }
    memset(udp_hints,0,sizeof (*udp_hints)); 
    udp_hints->ai_family=AF_INET; //IPv4 
    udp_hints->ai_socktype=SOCK_DGRAM;//UDP socket
    errcode = getaddrinfo(argv[3],argv[4],udp_hints,udp_serverInfo); 
    if(errcode!=0)
    {
        printf("Error: Unexpected getaddrinfo\n");
        exit(1);
    }



}

bool validate_inputArgs(int argc, char* argv[]){
    unsigned int adr1,adr2,adr3,adr4; //input address containers
    unsigned int port;
    char token[128];

    /*CHECK NUMBER OF ARGS*/
    if(argc != 5){
        printf("ERROR: Please insert a valid input argument list\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }

    /*CHECK IF THE TCP IP IS VALID*/
    if(sscanf(argv[1], "%u.%u.%u.%u%s", &adr1, &adr2, &adr3, &adr4, token)!=4){//returns the number of variables filled.
        printf("ERROR: Bad <ipaddr> -- That IP doesn't have a correct format\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
    else if( adr1>255 || adr2>255 || adr3>255 || adr4>255){//if 1 address > 255 (not a valid one)
        printf("ERROR: Bad <ipaddr> -- An IP address should not have a value over 255\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
    /*CHECK IF THE TCP PORT IS VALID*/
    if(sscanf(argv[2], "%u", &port)!=1 || port > 65535){
        printf("ERROR: Bad <tcpport> -- Port should not have a value over 65535\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }


    /*CHECK IF THE UDP IP IS VALID*/
    if(sscanf(argv[3], "%u.%u.%u.%u%s", &adr1, &adr2, &adr3, &adr4, token)!=4){
        printf("ERROR: Bad <reg_ipaddr> -- That IP doesn't have a correct format\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
    else if( adr1>255 || adr2>255 || adr3>255 || adr4>255){//if 1 address > 255 (not a valid one)
        printf("ERROR: Bad <reg_ipaddr> -- An IP address should not have a value over 255\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
    /*CHECK IF THE UDP PORT IS VALID*/
    if(sscanf(argv[4], "%u", &port)!=1 || port > 65535){
        printf("ERROR: Bad <reg_udpport> -- Port should not have a value over 65535\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
        
    printf("all validated, let's keep going\n\n");
    return true; //if nothing's wrong, return 1 == all validated
}

void prepare_tcpClient(struct addrinfo *cli_hints, struct addrinfo *cli_res, int* cli_fd){
    if(choose_extern()!=1)  //se for o primeiro no a ser registado salta 
    {
        //struct addrinfo hints, *res;
        int n;
        int var = 4;
        //ssize_t nbytes, nleft, nwritten, nread; 
        //char *ptr,buffer[128+1];

        *cli_fd = socket(AF_INET,SOCK_STREAM,0);//TCP socket 
        if(*cli_fd==-1)
        {
            printf("Error: Unexpected TCP_cli socket\n");
            exit(1); 
        }
        memset(cli_hints,0,sizeof (*cli_hints)); 
        cli_hints->ai_family=AF_INET;//IPv4
        cli_hints->ai_socktype=SOCK_STREAM;//TCP socket

        printf("Node IP/PORT%s %s\n\n", node.extern_IP, node.extern_PORT);
        n=getaddrinfo(node.extern_IP, node.extern_PORT, cli_hints, &cli_res);       //meter aqui o TCP do no escolhido 
        if(n!=0)
        {
            printf("Error: Unexpected getaddrinfo cli\n");
            exit(1);
        }
        //printf("cli_res->ai_addr %s\n", cli_res->ai_addr);
        n=connect(*cli_fd,cli_res->ai_addr,cli_res->ai_addrlen); 
        var = errno;
        printf("errono:: %d\n", var);
        if(n==-1)
        {
            printf("Error: Unexpected connect\n");
            //var = h_errno;
            printf("ERRNO: %d\n", var);
            exit(1);
        }
        else{
            printf("client successfully connected\n\n");

            memset(tcp_msg, 0, sizeof tcp_msg);//MEMSET Try
            sprintf(tcp_msg, "NEW %s %s\n", node.nodeIP, node.nodeTCP); //NEW IP TCP <LF>
            write(*cli_fd, tcp_msg, strlen(tcp_msg));

            memset(tcp_msg, 0, sizeof tcp_msg);

            read(*cli_fd, tcp_msg, BUFFER_SIZE);
            //Strtok extern e meter no backup com strcpy(node.extern_PORT, TCP_PORTarray[random_node]);
            printf("MSG from Server\n::%s\n", tcp_msg);
        }
    }
    return;
}

int choose_extern()
{
    int random_node;
    if (TCP_NodesCounter == 0)
    {
        //strcpy(node.extern_id = node.nodeid;
        strcpy(node.extern_IP, node.nodeIP);
        strcpy(node.extern_PORT, node.nodeTCP);

        //strcpy(node.backup_nid = node.nodeid;
        strcpy(node.backup_IP, node.nodeIP);
        strcpy(node.backup_PORT, node.nodeTCP);
        printf("estava sozinho\n");
        return(1);
    }

    else if (TCP_NodesCounter == 1)
    {
        //strcpy(node.extern_id = node.nodeid;
        strcpy(node.extern_IP, TCP_IParray[0]);
        strcpy(node.extern_PORT, TCP_PORTarray[0]);

        //strcpy(node.backup_nid = node.nodeid;
        strcpy(node.backup_IP, node.nodeIP);
        strcpy(node.backup_PORT, node.nodeTCP);

        return(0);
    }
    else{//CHANGE THIS FOR RANDOM CONNECT
        random_node = rand() % (TCP_NodesCounter-1);
        strcpy(node.extern_IP, TCP_IParray[random_node]);
        strcpy(node.extern_PORT, TCP_PORTarray[random_node]);

        /*strcpy(node.backup_IP, TCP_IParray[random_node]);
        strcpy(node.backup_PORT, TCP_PORTarray[random_node]);*/

    }
    
    return 0;
    
}

void prepare_tcpServer(struct addrinfo *ser_hints, struct addrinfo *ser_res, int *ser_listenfd){
    int errcode;
    
    if((*ser_listenfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        printf("Error: Unexpected TCP_serv socket\n");
        exit(1);
    }
    memset(ser_hints,0,sizeof (*ser_hints));
    ser_hints->ai_family=AF_INET;//IPv4
    ser_hints->ai_socktype=SOCK_STREAM;//TCP socket
    ser_hints->ai_flags=AI_PASSIVE; 
    if((errcode=getaddrinfo(NULL, node.nodeTCP,ser_hints,&ser_res))!=0)    //meter aqui o TCP do no escolhido 
    {
        printf("Error: Unexpected getaddrinfo serv\n");
        exit(1);
    }
    if(bind(*ser_listenfd, ser_res->ai_addr,ser_res->ai_addrlen)==-1)
    {
        printf("Error: Unexpected bind\n");
        exit(1);
    }
    if(listen(*ser_listenfd,5)==-1)
    {
        printf("Error: Unexpected listen\n");
        exit(1);
    }
    printf("Server side prepared\n");
}

void print_TCParray(){
    int i = 0;

    for(i = 0; i<TCP_NodesCounter; i++){
        printf("Array[%d]::  IP: %s   Port: %s\n\n", i, TCP_IParray[i], TCP_PORTarray[i]);
    }
}

void notreg_stdinCommands(char buffer[], char command[], char message[], int sockfd, struct addrinfo * udp_serverInfo){
    ssize_t n;

    //notreg_stdinCommands(buffer, command, message, sockfd, &state)
    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
    {
        
        if(sscanf(buffer, "%s", command)==1)
        {
            if(strcmp(command, "join")==0) 
            {
                printf("Command: join \n");
                if(sscanf(buffer, "%*s%s%s", node.netid, node.nodeid)!=2)
                    printf("Error: missing arguments\n");
                else if(!flag_list){ 
                    //new
                    sprintf(message, "NODES %s", node.netid);
                    n=sendto(sockfd,message, strlen(message), 0, udp_serverInfo->ai_addr, udp_serverInfo->ai_addrlen); 
                    if(n==-1)
                    {
                        printf("Error: Unexpected sendto\n");
                        exit(1);
                    }
                    state=listwait;
                }

                //resto das operacoes necessarias
            }
            else if (strcmp(command, "leave")==0) printf("Makes no sense to leave before joining \n");
            else if (strcmp(command, "exit")==0) {printf("Going out.\n\n"); state=goingout;}
            //resto dos comandos possiveis
            //...
            else printf("Error: Unknown command. \n");
        }//if command                    
    }//fgets

    if(state!=goingout && flag_list==0)printf("ndn> "); fflush(stdout); //prompt
}

void udp_regRequest(char message[], struct addrinfo * udp_serverInfo, int sockfd){
    int n;

    sprintf(message, "REG %s %s %s", node.netid, node.nodeIP, node.nodeTCP);
    n=sendto(sockfd,message, strlen(message), 0, udp_serverInfo->ai_addr, udp_serverInfo->ai_addrlen); 
    if(n==-1)
    {
        printf("Error: Unexpected sendto\n");
        exit(1);
    }
    
    flag_list = 0;
    state=regwait;
}

void regwait_stdinCommands(char buffer[], char command[]){
    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
    {
        if(sscanf(buffer, "%s", command)==1)
        {
            if(strcmp(command, "join")==0) printf("Join in progress... \n");
            else if (strcmp(command, "leave")==0) printf("Makes no sense to leave before joining \n");
            else if (strcmp(command, "exit")==0) {printf("Going out.\n\n"); state=goingout;}
            //resto dos comandos possiveis
            //...
            else printf("Error: Unknown command. \n");
        }//if commands
    }
    if(state!=goingout)printf("ndn> "); fflush(stdout); //prompt
}

void rcv_OkReg(int sockfd, char message[], struct sockaddr * server_addr, socklen_t * addrlen, struct addrinfo * ser_hints,
struct addrinfo * ser_res, int * ser_listenfd){
    ssize_t n;
    n=recvfrom(sockfd, message, BUFFER_SIZE-1, 0, server_addr, addrlen); 
    if(n==-1)
    {
        printf("Error: Unexpected recvfrom\n");
        exit(1);
    }
    message[n]='\0';
    if (strcmp(message, "OKREG")==0){
        printf("Registered.\nndn>");
        fflush(stdout);
        prepare_tcpServer(ser_hints, ser_res, ser_listenfd);//SERVER PREPARED
        state=reg;
    } //esta registado
}

void reg_stdinCommands(char buffer[], char command[], char message[],int sockfd, struct addrinfo * udp_serverInfo){
    ssize_t n;
    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
    {
        if(sscanf(buffer, "%s", command)==1)
        {
            if(strcmp(command, "join")==0) printf("Already joined. \n");
            else if (strcmp(command, "leave")==0) 
            {
                sprintf(message, "UNREG %s %s %s", node.netid, node.nodeIP, node.nodeTCP);
                n=sendto(sockfd,message, strlen(message), 0, udp_serverInfo->ai_addr, udp_serverInfo->ai_addrlen); 
                if(n==-1)
                {
                    printf("Error: Unexpected sendto\n");
                    exit(1);
                }
                state=notregwait;
            }
            else if (strcmp(command, "exit")==0)
            {
                printf("Going out.\n\n");
                state=goingout;
            }
            else if (strcmp(command, "st")==0) 
            {
                printf("External neighbour's IP - %s\n", node.extern_IP);
                printf("External neighbour's TCP - %s\n", node.extern_PORT);
                printf("Backup's IP - %s\n", node.backup_IP);
                printf("Backup's TCP - %s\n", node.backup_PORT);
            }
            //resto dos comandos possiveis
            //...
            else printf("Error: Unknown command. \n");
            
        }//if command
    }
    if(state!=goingout)printf("ndn> "); fflush(stdout); //prompt
}

void rcv_newCLient(struct sockaddr *myclient_addr,int myclient_fd, int ser_listenfd){
    clientNode *new_node = NULL;
    socklen_t addrlen = sizeof(myclient_addr);

    if((myclient_fd=accept(ser_listenfd, myclient_addr, &addrlen))==-1)//accept the new client
    {
        printf("Error: Unexpected accept\n");
        exit(1);
    }
    else{
        new_node = alloc_clientNode(myclient_fd);//alloc || insert in clientsList
        clients_OnLine++;
        memset(tcp_msg, 0, sizeof tcp_msg);
        read(new_node->fd, tcp_msg, BUFFER_SIZE);
        printf("Msg from Client\n ::%s\n", tcp_msg);

        memset(tcp_msg, 0, sizeof tcp_msg);//MEMSET TRY

        sprintf(tcp_msg, "EXTERN %s %s\n", node.backup_IP, node.backup_PORT); //EXTERN IP TCP<LF> //prof
        write(new_node->fd, tcp_msg, strlen(tcp_msg));//prof
    }
}

void rcv_msgFromServer(int cli_fd, char return_message[]){
    memset(return_message, 0, sizeof return_message);//MEMSET TRY
    ssize_t n=read(cli_fd,return_message,BUFFER_SIZE);
    if(n==-1){
        printf("Error rcving the server message\n\n");
        exit(1);
    }
    else{//tudo ok
        printf("Server Message:\n %s", return_message);
    }
}

void rcv_msgFromClients(int myclient_fd, char return_message[]){
    ssize_t n=read(myclient_fd,return_message,BUFFER_SIZE);
    if(n==-1){
        printf("Error rcving the server message\n\n");
        exit(1);
    }
    else{//tudo ok
        printf("Client Message\n %s", return_message);
    }
}

void notregwait_stdinCommands(char buffer[], char command[]){
    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
    {
        if(sscanf(buffer, "%s", command)==1)
        {
            if(strcmp(command, "join")==0) printf("Leave in progress... \n");
            else if (strcmp(command, "leave")==0) printf("Leave in progress... \n");
            else if (strcmp(command, "exit")==0) {printf("Going out.\n\n"); state=goingout;}
            //resto dos comandos possiveis
            //...
            else printf("Error: Unknown command. \n");
        }//if commands
    }
    if(state!=goingout)printf("ndn> "); fflush(stdout); //prompt
}

void rcv_OkUnreg(int sockfd, char message[], struct sockaddr * server_addr, socklen_t * addrlen){
    ssize_t n=recvfrom(sockfd, message, BUFFER_SIZE-1, 0, server_addr, addrlen); 
    if(n==-1)
    {
        printf("Error: Unexpected recvfrom\n");
        exit(1);
    }
    message[n]='\0';
    if (strcmp(message, "OKUNREG")==0){
        printf("Unregistered.\nndn>");
        fflush(stdout);
        state=notreg;
        } 
}

void listwait_stdinCommands(char buffer[], char command[]){
    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
    {
        if(sscanf(buffer, "%s", command)==1)
        {
            if(strcmp(command, "join")==0) printf("Join in progress... \n");
            else if (strcmp(command, "leave")==0) printf("Makes no sense to leave before joining \n");
            else if (strcmp(command, "exit")==0) {printf("Going out.\n\n"); state=goingout;}
            //resto dos comandos possiveis
            //...
            else printf("Error: Unknown command. \n");
        }//if commands
    }
    if(state!=goingout)
        printf("ndn> ");
    fflush(stdout); //prompt
}

void rcv_nodeslist(int sockfd, char message[], struct sockaddr * server_addr, socklen_t * addrlen, char message1[], char* token){
    ssize_t n = recvfrom(sockfd, message, BUFFER_SIZE-1, 0, server_addr, addrlen); 
    if(n==-1)
    {
        printf("Error: Unexpected recvfrom\n");
        exit(1);
    }
    message[n]='\0';

    if (sscanf(message, "%s", message1)==1)
    { 
        if (strcmp(message1, "NODESLIST")==0)
        {   
            token = strtok(message, "\n");//get the "NODESLIST\n"

            while(token != NULL && TCP_NodesCounter<NODELIST_SIZE){
                token = strtok(NULL, " ");//get TCP IP
                if(token != NULL){//if we don't have any node in the list
                    sscanf(token, "%s", TCP_IParray[TCP_NodesCounter]);
                    token = strtok(NULL, "\n");//get TCP PORT
                    sscanf(token, "%s", TCP_PORTarray[TCP_NodesCounter]);
                    //here a new node is already in the array
                    TCP_NodesCounter++;
                }
            }

            printf("List Received.\nndn>"); 
            print_TCParray();
            fflush(stdout);
        } //lista recebida
        else printf("Error: Return message descriptor . \n");
    }
    else printf("Error: Return NODESLIST message . \n");

    flag_list = 1;
    //printf("VAI PARA O NOTREG \n");
    state=notreg;
}

clientNode* alloc_clientNode(int client_fd){ 
    clientNode * new_clientNode = (clientNode*) malloc(sizeof(clientNode));

    new_clientNode->next=NULL;
    new_clientNode->fd = client_fd;
    insertNode_clientsList(new_clientNode);

    return new_clientNode; 
}

void free_clientNode(clientNode * node){
    free(node);
}

void insertNode_clientsList(clientNode *new_node){

    clientNode* aux = clientsList_head;

    if(clientsList_head == NULL){//if the new node is the 1st one
        clientsList_head = new_node;
    }
    else{
        while(aux->next != NULL){
            aux=aux->next;
        }//get the last node of the list

        aux->next = new_node;
    }

}

int get_ClientsMaxfd(){
    clientNode* aux = clientsList_head;
    int max_fd = 0;

    if(aux->next == NULL){//if only 1 node in the list
        max_fd = aux->fd;
        return max_fd;
    }

    while(aux->next !=NULL){
        if(max_fd < aux->fd)
            max_fd = aux->fd;

        aux=aux->next;
    }

    return max_fd;
}

void prepare_ClientFDSets(fd_set * rfds){
    clientNode* aux = clientsList_head;

    while(aux != NULL){
        FD_SET(aux->fd, rfds);
        aux=aux->next;
    }
}

int get_ClientISSET(fd_set * rfds){
    clientNode* aux = clientsList_head;

    while(aux!=NULL){
        if(FD_ISSET(aux->fd,rfds)){
            FD_CLR(aux->fd,rfds);
            return aux->fd;
        }
        aux=aux->next;
    }

    return -1;//if no FD did ISSET
}