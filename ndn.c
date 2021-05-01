#include "headerFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



/************** VARIÁVEIS GLOBAIS **************/
enum {notreg, goingout, regwait, reg, notregwait, listwait} state;

node_s node;
int flag_list = 0;
char TCP_IParray[NODELIST_SIZE][BUFFER_SIZE];//5 for now
char TCP_PORTarray[NODELIST_SIZE][BUFFER_SIZE];
int TCP_NodesCounter = 0;

int expTable[TABLE_SIZE] = {0};//Tabela de expedição -- contém fd associado a cada id || -1 represent 

////////NEW BUFFERS
char newClientBuffer[BUFFER_SIZE];
char serverMessageBuffer[BUFFER_SIZE];

char tcp_msg[BUFFER_SIZE];
char return_message[BUFFER_SIZE];

clientNode *clientsList_head = NULL;
int clients_OnLine = 0; //number of clients On server's Line
int var = 0;

int advertise_counter=0; //differntiate 1st advertise from the others to fill node structure
int flag_ex=0; //if node just entered network (0) or if its just choosing a new extern (1) -- used in prepareTCP_Client
int im_alone = 0;

/***************** MAIN *****************/
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
    int cli_fd =-1;

    /*TCP SERVER VARIABLES*/
    struct addrinfo ser_hints;
    struct addrinfo *ser_res;
    int ser_listenfd;
    int ser_acceptfd;
    int myclient_fd;
    struct sockaddr myclient_addr;

    /*****************************/
    /*****PROGRAM STARTS HERE*****/
    /*****************************/

    /*INPUT VALIDATION*/
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

            case reg:        FD_SET(0, &rfds);  FD_SET(ser_listenfd, &rfds);//prepare server, listen & stdin FD
                maxfd=ser_listenfd;
                if(clientsList_head!=NULL){//if CLIENTS CONNECTED -- update maxfd and prepare clients FDSets
                    prepare_ClientFDSets(&rfds);
                    maxfd = get_ClientsMaxfd();//warning prof
                }
                if(cli_fd!=-1){
                    FD_SET(cli_fd, &rfds);
                    if(maxfd < cli_fd)
                        maxfd = cli_fd;
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
                    notreg_stdinCommands(buffer, command, message, sockfd, udp_serverInfo, &cli_hints, cli_res, &cli_fd);
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
                    reg_stdinCommands(buffer, command, message, sockfd, udp_serverInfo, cli_fd, ser_listenfd);
                }//stdin

                /******* SERVER recebe NEW CLIENT ******/
                else if (FD_ISSET(ser_listenfd,&rfds)){ //if a new client has connected
                    FD_CLR(ser_listenfd,&rfds);
                    rcv_newCLient(&myclient_addr, myclient_fd, ser_listenfd, &cli_fd, &cli_hints, cli_res);
                }
                /******* CLIENT a receber do SERVER *******/
                else if (FD_ISSET(cli_fd,&rfds)){ 
                    FD_CLR(cli_fd,&rfds);
                    rcv_msgFromServer(&cli_fd, &cli_hints, cli_res); 
                }
                
                /********** SERVER recebe dos seus CLIENTs *******/
                else{
                    target_fd = get_ClientISSET(&rfds);
                    if(target_fd != -1){//if we have some fd ISSET
                        rcv_msgFromClients(target_fd, cli_fd);
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
    
    char writeBuffer[BUFFER_SIZE]; 

    int choose = 0;
    if (flag_ex==0)
    {
        choose = choose_extern();
    }
     
    if(choose!=1)  //se for o primeiro no a ser registado salta 
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

        printf("Connecting to Node IP/PORT%s %s as extern...\n\n", node.extern_IP, node.extern_PORT);
        n=getaddrinfo(node.extern_IP, node.extern_PORT, cli_hints, &cli_res);       //meter aqui o TCP do no escolhido 
        if(n!=0)
        {
            printf("Error: Unexpected getaddrinfo cli\n");
            exit(1);
        }
        //printf("cli_res->ai_addr %s\n", cli_res->ai_addr);
        n=connect(*cli_fd,cli_res->ai_addr,cli_res->ai_addrlen); 
        var = errno;
        if(n==-1)
        {
            printf("Error: Unexpected connect\n");
            //var = h_errno;
            printf("ERRNO: %d\n", var);
            exit(1);
        }
        else{
            printf("client successfully connected\n\n");
            memset(writeBuffer, 0, sizeof writeBuffer);
            sprintf(writeBuffer, "NEW %s %s\n", node.nodeIP, node.nodeTCP); //NEW IP TCP <LF>
            write(*cli_fd, writeBuffer, strlen(writeBuffer));   
        }
    }
    return;
}

int choose_extern()
{
    int random_node;
    if (TCP_NodesCounter == 0)
    {
        strcpy(node.extern_id, node.nodeid);
        strcpy(node.extern_IP, node.nodeIP);
        strcpy(node.extern_PORT, node.nodeTCP);

        strcpy(node.backup_id, node.nodeid);
        strcpy(node.backup_IP, node.nodeIP);
        strcpy(node.backup_PORT, node.nodeTCP);
        printf("estava sozinho\n");
        im_alone = 1;
        return(1);
    }

    else if (TCP_NodesCounter == 1)
    {
        //strcpy(node.extern_id, node.nodeid);
        strcpy(node.extern_IP, TCP_IParray[0]);
        strcpy(node.extern_PORT, TCP_PORTarray[0]);

        //strcpy(node.backup_id, node.nodeid);
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

void notreg_stdinCommands(char buffer[], char command[], char message[], int sockfd, struct addrinfo * udp_serverInfo,struct addrinfo *cli_hints, struct addrinfo *cli_res, int* cli_fd){
    ssize_t n;

    //notreg_stdinCommands(buffer, command, message, sockfd, &state)
    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
    {
        
        if(sscanf(buffer, "%s", command)==1)
        {
            if(strcmp(command, "join")==0) 
            {
                printf("Command: join \n");
                if(sscanf(buffer, "%*s%s%s%s%s", node.netid, node.nodeid, node.extern_IP, node.extern_PORT)==2){//if join net id
                    if(!flag_list){ 
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
                }
                else if(sscanf(buffer, "%*s%s%s%s%s", node.netid, node.nodeid, node.extern_IP, node.extern_PORT)==4){//join net id bootIP bootTCP
                    flag_ex=1;//do not assign his extern
                    prepare_tcpClient(cli_hints, cli_res, cli_fd);
                    state=reg;
                }
                else{
                    printf("Missing join arguments. Options::\n join net id\njoin net id bootIP bootTCP\n");
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

        TCP_NodesCounter++;

        prepare_tcpServer(ser_hints, ser_res, ser_listenfd);//SERVER PREPARED
        state=reg;
    } //esta registado
}

void reg_stdinCommands(char buffer[], char command[], char message[],int sockfd, struct addrinfo * udp_serverInfo, int cli_fd, int ser_listenfd){
    ssize_t n;
    clientNode* aux = clientsList_head;//aponta pro inicio da lista dos clients
    clientNode* free_aux = aux;
    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
    {
        if(sscanf(buffer, "%s", command)==1)
        {
            if(strcmp(command, "join")==0) printf("Already joined. \n");
            else if (strcmp(command, "leave")==0) 
            {
                //close TCP connections to clients
                closenFree_clientsList();

                //close remaining TCP connections 
                close(cli_fd);//extern connection
                close(ser_listenfd);
                
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
            else if (strcmp(command, "sr")==0) 
            {
                for(int i = 0; i < TABLE_SIZE; i++){
                    if(expTable[i]!=0)
                        printf("Node %d - %d\n", i, expTable[i]);
                } 
            }
            //resto dos comandos possiveis
            //...
            else printf("Error: Unknown command. \n");
        }//if command
    }
    if(state!=goingout)printf("ndn> "); fflush(stdout); //prompt
}

void rcv_newCLient(struct sockaddr *myclient_addr,int myclient_fd, int ser_listenfd, int* cli_fd, struct addrinfo *cli_hints, struct addrinfo *cli_res){
    clientNode *new_node = NULL;
    socklen_t addrlen = sizeof(myclient_addr);
    char writeBuffer [BUFFER_SIZE];
    memset(writeBuffer,0, sizeof writeBuffer);
    char* token = NULL;
    char* token1 = NULL;
    char* token2 = NULL;

    int advertise_id = -1;
    int n_written;
    char msg_code[BUFFER_SIZE];
    memset(msg_code, 0, sizeof (msg_code));
    char auxIP[BUFFER_SIZE];
    memset(auxIP, 0, sizeof (auxIP));
    char auxPORT[BUFFER_SIZE];
    memset(auxPORT, 0, sizeof (auxPORT));
    clientNode *aux = clientsList_head;
    int op;

    //Accept the new client
    if((myclient_fd=accept(ser_listenfd, myclient_addr, &addrlen))==-1)
    {
        printf("Error: Unexpected accept\n");
        exit(1);
    }

    else{
        read(myclient_fd, newClientBuffer, BUFFER_SIZE-1);// NEW IP TCP<LF>
        printf("Msg from Client\n ::%s\n", newClientBuffer);
        if (sscanf(newClientBuffer, "%s", msg_code)==1){ 

            if (strcmp(msg_code, "NEW")==0){
                token = strtok(newClientBuffer, " ");//get NEW
                token1 = strtok(NULL, " ");//get IP
                strcpy(auxIP, token1);

                token2 = strtok(NULL, "\n");//get TCP PORT
                strcpy(auxPORT, token2);
                
                if(token1 == NULL || token2 == NULL){
                    printf("NEW received with wrong format\n");
                }
                else{ //mensagem completa
                    new_node = alloc_clientNode(myclient_fd, token1, token2);//alloc || insert in clientsList
                    clients_OnLine++;
                    
                    //If it is the only node in the network
                    if( (strcmp(node.extern_IP, node.nodeIP)==0) && (strcmp(node.extern_PORT, node.nodeTCP)==0)){ //se este node for o primeiro da rede aka seu externo é ele proprio
                        strcpy(node.extern_IP, token1);
                        strcpy(node.extern_PORT, token2);
                    }
                    
                    //LEAVE
                    /*if(strlen(node.extern_IP) == 0){//if i don't have an extern, assign one of my clients as my extern
                        flag_ex=1;
                        extern_randomClient();
                        printf("i don't have any extern node \n connecting to %s %s \n", node.extern_IP, node.extern_PORT);
                        prepare_tcpClient(cli_hints, cli_res, cli_fd);
                    }*/

                    memset(writeBuffer, 0, sizeof writeBuffer);
                    sprintf(writeBuffer, "EXTERN %s %s\n", node.extern_IP, node.extern_PORT);// EXTERN IP TCP<LF> 
                    write(new_node->fd, writeBuffer, strlen(writeBuffer));
                    printf("sent to myNEWclient:: %s\n", writeBuffer);

                    //Advertise itself to new client
                    memset(writeBuffer, 0, sizeof writeBuffer);
                    sprintf(writeBuffer, "ADVERTISE %s\n", node.nodeid);
                    n_written=write(new_node->fd, writeBuffer, strlen(writeBuffer));
                    printf("sent to myNEWclient:: %s\n", writeBuffer);
                    if(n_written <= 0){
                        printf("Error writing advertise b4 for loop\n");
                    }

                    //Advertise each table element to the new client
                    for(int i = 0; i < TABLE_SIZE; i++){
                        if(expTable[i] != 0){
                            memset(writeBuffer, 0, sizeof writeBuffer);
                            sprintf(writeBuffer, "ADVERTISE %d\n", i);
                            write(new_node->fd, writeBuffer, strlen(writeBuffer));
                            printf("sent to my New client:%s\n", writeBuffer);
                        }
                    }
                }
            }
        }
    }
    memset(newClientBuffer, 0, sizeof (newClientBuffer));
}

void rcv_msgFromServer(int *cli_fd, struct addrinfo *cli_hints, struct addrinfo *cli_res){  //CLI_FD TALVEZ TENHA QUE SER COM &
    char msg_code[BUFFER_SIZE];
    char id_char[BUFFER_SIZE];

    char *token;
    char *token1;
    char *token2;

    int advertise_id = -1;
    int withdraw_id = -1;

    char writeBuffer[BUFFER_SIZE];
    char auxBuffer[BUFFER_SIZE];
    int msg_len = 0;

    ssize_t n=read(*cli_fd,serverMessageBuffer,BUFFER_SIZE-1);
    strcpy(auxBuffer,serverMessageBuffer);

    if(n==-1 || n == 0){
        printf("My extern node left\n");

        //external neighbour WITHDRAWs
        if (strcmp(node.backup_IP,node.nodeIP) != 0 || strcmp(node.backup_PORT,node.nodeTCP) != 0)//if this node IS NOT his own backup
        {
        //set the backup as my extern
            strcpy(node.extern_IP, node.backup_IP);
            strcpy(node.extern_PORT, node.backup_PORT);
            memset(node.backup_IP, 0, sizeof (node.backup_IP));
            memset(node.backup_PORT, 0, sizeof(node.backup_PORT));

            clean_expTable(*cli_fd);//percorrer tabela, achar ID desse FD, enviar withdraw a todos os seu clientes
            close (*cli_fd);
            *cli_fd = -1;
            
            flag_ex = 1; //do not change my extern in tpcClient
            prepare_tcpClient(cli_hints, cli_res, cli_fd);
        }
        else{//if he IS his own backup
//Não entra pq não temos em conta essa opção de ele tentar ler do cliente e verificar-se que ele é tb teu servidor
            clean_expTable(*cli_fd);
            close (*cli_fd);
            *cli_fd = -1;
            
            if(clients_OnLine > 0){
                extern_randomClient(); //escolhe 1 dos seus clientes e preenche o seu node.extern
                memset(node.backup_IP, 0, sizeof (node.backup_IP));
                memset(node.backup_PORT, 0, sizeof(node.backup_PORT));
                flag_ex = 1; //do not change my extern in tcpClient
                prepare_tcpClient(cli_hints, cli_res, cli_fd);
            }
            else{ //no clients available --> meter o seu extern e backup a seu próprio
                strcpy(node.extern_IP, node.nodeIP);
                strcpy(node.extern_PORT, node.nodeTCP);
            }

        }
    }

    else{//tudo ok
        printf("Server Message:\n %s\n", serverMessageBuffer);

        while (sscanf(auxBuffer, "%s", msg_code)==1){
            if(strcmp(msg_code, "EXTERN")==0){ //EXTERN IP TCP<LF>
                msg_len = 0;
                token = strtok(auxBuffer, " ");//get EXTERN
                msg_len += strlen(token)+1;

                token1 = strtok(NULL, " "); //get IP
                msg_len += strlen(token1)+1;
                
                token2 = strtok(NULL, "\n");//get PORT
                msg_len += strlen(token2)+1;

                //LEAVE Case -- it has clients and the backup is not up to date
                if (clientsList_head!=NULL && (strcmp(token1, node.backup_IP)!=0) && (strcmp(token2, node.backup_PORT)!=0)){//se tiver clientes
                    send_ToMyClients(0, -1, 2); //if it has clients propagates his EXTERN to them
                }
                
                sscanf(token1, "%s", node.backup_IP);//save it
                sscanf(token2, "%s", node.backup_PORT);//save it

                memcpy(auxBuffer, &auxBuffer[msg_len], (BUFFER_SIZE - msg_len)*sizeof(char));//SHIFT auxBUFFER

                //Advertise itself to external neighbour aka server node
                memset(writeBuffer, 0, sizeof(writeBuffer));
                sprintf(writeBuffer, "ADVERTISE %s\n", node.nodeid);
                write(*cli_fd, writeBuffer, strlen(writeBuffer));
                printf("sent to myserver::%s\n", writeBuffer);

            }
            else if(strcmp(msg_code, "ADVERTISE") == 0){//ADVERTISE id<LF>
                msg_len = 0;
                memset(id_char, 0, sizeof(id_char));

                token = strtok(auxBuffer, " ");//get ADVERTISE
                msg_len += strlen(token)+1;

                token = strtok(NULL, "\n");//get id
                msg_len += strlen(token)+1;
                sscanf(token, "%s", id_char); 

                if(strcmp(id_char, node.nodeid) != 0){ //if the advertised one is not him
                    advertise_id = atoi(id_char);
                //LEAVE
                    if(expTable[advertise_id]!=*cli_fd){//se o ADVER não vier do server
                        expTable[advertise_id] = *cli_fd;
                        send_ToMyClients(advertise_id, -1, 1);//send to all his clients
                    }
                }
                memcpy(auxBuffer, &auxBuffer[msg_len], (BUFFER_SIZE - msg_len)*sizeof(char));//SHIFT auxBUFFER
            }
            else if(strcmp(msg_code, "WITHDRAW") == 0){//WITHDRAW id<LF>
                msg_len = 0;
                memset(id_char, 0, sizeof(id_char));

                token = strtok(auxBuffer, " ");//get WITHDRAW
                msg_len += strlen(token)+1;

                token = strtok(NULL, "\n");//get id
                msg_len += strlen(token)+1;
                sscanf(token, "%s", id_char);  

                withdraw_id = atoi(token);
                expTable[withdraw_id] = 0;
                send_ToMyClients(withdraw_id, -1, -1);//send to all his clients

                memcpy(auxBuffer, &auxBuffer[msg_len], (BUFFER_SIZE - msg_len)*sizeof(char));//SHIFT auxBUFFER
            }
        }
    }
    memset(serverMessageBuffer, 0, sizeof (serverMessageBuffer));
}

void rcv_msgFromClients(int myclient_fd, int cli_fd){
    clientNode *aux = clientsList_head; 
    clientNode *myclient = NULL;

    char * token=NULL;
    int advertise_id= -1;
    int withdraw_id= -1;
    int left_nodeID = -1;

    int op=0; //distinguishes between ADVERTISE(1), WITHDRAW(-1) etc

    char msg_code[BUFFER_SIZE];
    char id_char[BUFFER_SIZE];
    char auxBuffer[BUFFER_SIZE];
    memset(auxBuffer, 0, sizeof (auxBuffer));
    char writeBuffer[BUFFER_SIZE];
    memset(writeBuffer, 0, sizeof(writeBuffer));
    int msg_len = 0;
    
    //myclient = getClientBuffer(myclient_fd);
    //if(myclient==NULL) printf("Error: No client structure found\n");


    ssize_t n=read(myclient_fd,auxBuffer, BUFFER_SIZE-1);
    //auxBuffer[BUFFER_SIZE-1] = '\0';

    if(n==-1 || n == 0){
        printf("My inner node left\n");


        //VER CASO 2 baze e 1 fique (extern do 1)
        rmvNode_clientsList(myclient_fd);
        clients_OnLine--;
        clean_expTable2(myclient_fd, cli_fd);//percorrer tabela, achar ID desse FD, enviar withdraw a todos os seu clientes && //withdraw my server

        if(clients_OnLine == 0 && cli_fd == -1){//if no more clients and servers available
            strcpy(node.extern_IP, node.nodeIP);
            strcpy(node.extern_PORT, node.nodeTCP);
        }
    }
    
    else{//tudo ok
        printf("Client Message:\n%s\n", auxBuffer);
        
        while(sscanf(auxBuffer, "%s", msg_code)==1){
            if(strcmp(msg_code, "ADVERTISE") == 0){//ADVERTISE id<LF>
                msg_len = 0;
                memset(id_char, 0, sizeof(id_char));

                token = strtok(auxBuffer, " ");//get ADVERTISE
                msg_len += strlen(token)+1;

                token = strtok(NULL, "\n");//get id
                msg_len += strlen(token)+1;
                sscanf(token, "%s", id_char);

                if(strcmp(id_char, node.nodeid)  != 0){ //if the advertised one is not him
                    advertise_id = atoi(token);

                    if(expTable[advertise_id]!=myclient_fd){//se a tabela não está atualizada
                        expTable[advertise_id] = myclient_fd;
                        send_ToMyClients(advertise_id, myclient_fd, 1);//send to all his clients except the 1 that sent the advertise
                        //send the advertise to his server
                        if(cli_fd!=-1){
                            memset(writeBuffer, 0, sizeof(writeBuffer));
                            sprintf(writeBuffer, "ADVERTISE %s\n", id_char);
                            write(cli_fd, writeBuffer, strlen(writeBuffer));
                            printf("sent to myServer::%s\n", writeBuffer);
                        }
                    }
                }
                memcpy(auxBuffer, &auxBuffer[msg_len], (BUFFER_SIZE - msg_len)*sizeof(char));//SHIFT auxBUFFER       
            }
            else if(strcmp(msg_code, "WITHDRAW") == 0){//WITHDRAW id<LF>
                msg_len = 0;
                memset(id_char, 0, sizeof(id_char));
                
                token = strtok(auxBuffer, " ");//get WITHDRAW
                msg_len += strlen(token)+1;

                token = strtok(NULL, "\n");//get id
                msg_len += strlen(token)+1;
                sscanf(token, "%s", id_char);
                withdraw_id = atoi(id_char);

                if(expTable[withdraw_id]!=0){//if it is not updated
                    expTable[withdraw_id] = 0;
                    send_ToMyClients(withdraw_id, myclient_fd, -1);//send to all his clients except the 1 that sent the advertise
                }

                memcpy(auxBuffer, &auxBuffer[msg_len], (BUFFER_SIZE - msg_len)*sizeof(char));//SHIFT auxBUFFER
            }
        }
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

clientNode* alloc_clientNode(int client_fd, char * ipp, char * tcpp){ 
    clientNode * new_clientNode = (clientNode*) malloc(sizeof(clientNode));

    new_clientNode->next=NULL;
    new_clientNode->fd = client_fd;
    strcpy(new_clientNode->ip, ipp);
    strcpy(new_clientNode->tcp, tcpp);
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

    while(aux !=NULL){
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

void net_reroute(){
    
    //close sockets and update node structure 
    if(node.nodeIP == node.backup_IP){
        //promove qualquer um dos seus vizinhos internos a vizinho externo:
        // - 
        
    }
    else{
        
        
    }
}

void prepare_myExternExit(){
    if(!(strcmp(node.nodeIP, node.backup_IP) && node.nodeTCP == node.backup_PORT)){//if he's not his own backup

    }
    

}

void send_ToMyClients(int adver_id, int font_fd, int op){//font_fd(-1) -- send 2 all clients || font_fd(!=-1) -- send to all client except the font_fd 
    clientNode* aux = clientsList_head;
    char msg_buffer[BUFFER_SIZE];

    if (op==1) //ADVERTISE
    {
        if(font_fd == -1){//if we want to send to all clients
            while(aux!=NULL){
                memset(msg_buffer, 0, sizeof msg_buffer);
                sprintf(msg_buffer, "ADVERTISE %d\n", adver_id);
                write(aux->fd, msg_buffer, strlen(msg_buffer));

                aux=aux->next;
            }
        }
        else{ //if we do not want to send to the font_fd node
            while(aux!=NULL){
                if(aux->fd != font_fd){
                    memset(msg_buffer, 0, sizeof msg_buffer);
                    sprintf(msg_buffer, "ADVERTISE %d\n", adver_id);
                    write(aux->fd, msg_buffer, strlen(msg_buffer));
                }
                aux=aux->next;
            }
        }
    }

    else if (op==-1)//WITHDRAW
    {
        if(font_fd == -1){//if we want to send to all clients
            while(aux!=NULL){
                memset(msg_buffer, 0, sizeof msg_buffer);
                sprintf(msg_buffer, "WITHDRAW %d\n", adver_id);
                write(aux->fd, msg_buffer, strlen(msg_buffer));

                aux=aux->next;
            }
        }
        else{ //if we do not want to send to the font_fd node
            while(aux!=NULL){
                if(aux->fd != font_fd){
                    memset(msg_buffer, 0, sizeof msg_buffer);
                    sprintf(msg_buffer, "WITHDRAW %d\n", adver_id);
                    write(aux->fd, msg_buffer, strlen(msg_buffer));
                }
                aux=aux->next;
            }
        }
    }

    else if (op==2){ //EXTERN
        while(aux!=NULL){
            memset(msg_buffer, 0, sizeof msg_buffer);
            sprintf(msg_buffer, "EXTERN %s %s\n", node.extern_IP, node.extern_PORT);
            write(aux->fd, msg_buffer, strlen(msg_buffer));
            aux=aux->next;
        }
    }   
}

void advertise_myExternalNode(int advertise_id, int cli_fd){
    char msg[BUFFER_SIZE];

    if(cli_fd!=-1){//if there's an external node
        sprintf(msg, "ADVERTISE %d\n", advertise_id);
        int n_written=write(cli_fd, msg, strlen(msg));
        if(n_written <= 0){
            printf("Error wrting advertise\n");
        }
    }
}

void closenFree_clientsList(){
    clientNode * aux = clientsList_head;
    clientNode * free_aux = aux;

    while (aux!=NULL){
        close(aux->fd);
        if(aux->next != NULL){//if not the first node
            free_aux = aux;
            aux = aux->next;//avança prox nó
            free(free_aux);
        }
        else if (aux->next == NULL){//if the 1st node
            aux = aux->next;
            free(clientsList_head);//free the 1st node
        }
    }
}

int get_idFromExpTable(int my_client_fd){

    for(int i = 0; i< TABLE_SIZE; i++){
        if(expTable[i] == my_client_fd)
            return i;
    }
    return -1;
}

void extern_randomClient(){
    clientNode * aux = clientsList_head; //get the head of the list
    int random_number = rand() % (clients_OnLine-1);
    int iterator = 0;

    while(aux != NULL){
        if(iterator == random_number){
            strcpy(node.extern_IP, aux->ip);
            strcpy(node.extern_PORT, aux->tcp);
            printf("new random extern assigned:: %s %s\n", node.extern_IP, node.extern_PORT);
        }
        iterator++;
        aux=aux->next;
    }
}

void rmv_thisClient_fromtheList(int rmv_fd){
    clientNode * aux = clientsList_head;
    clientNode * free_aux = aux;
    int iterator = 0;

    while(aux!=NULL){
        if(aux->fd == rmv_fd){
            free_aux = aux;
            aux=aux->next;
            iterator++;
            close(free_aux->fd);
            free(free_aux);
            if(iterator == 1)//if the 1st node was rmved, update the clientList_head
                clientsList_head = aux;
        }
        else{
            aux=aux->next;
            iterator++;
        }
    }
}

void compare_externBackupNodes(int left_nodeID){
    clientNode * aux = clientsList_head;

    while(aux!=NULL){
        if(aux->fd == expTable[left_nodeID]){//find the left IP and PORT
            break;
        }
    aux=aux->next;
    }
    //aux has the left_node

    if( (strcmp(node.extern_IP,aux->ip)==0) && (strcmp(node.extern_PORT, aux->tcp)==0) ){//if he IS my EXTERN
        //clean the extern and wait for the NEW
        memset(node.extern_IP, 0, sizeof node.extern_IP);
        memset(node.extern_PORT, 0, sizeof node.extern_PORT);
        memset(node.extern_id, 0, sizeof node.extern_id);
    }
}

void clean_expTable(int left_fd){
    
    for(int i = 0; i< TABLE_SIZE; i++){
        if(expTable[i] == left_fd){
            //send withdraw to my clients
            send_ToMyClients(i,-1,-1);//withdraw to all clients

            //clean Table
            expTable[i] = 0;
        }
    }
}

void clean_expTable2(int left_fd, int cli_fd){
    char writeBuff[BUFFER_SIZE];
    
    for(int i = 0; i< TABLE_SIZE; i++){
        if(expTable[i] == left_fd){
            //send withdraw to my clients
            send_ToMyClients(i,-1,-1);//withdraw to all clients

            //withdraw to his server
            if(cli_fd != -1){
                memset(writeBuff, 0, sizeof(writeBuff));
                sprintf(writeBuff,"WITHDRAW %d\n", i);
                write(cli_fd, writeBuff, strlen(writeBuff));
            }

            //clean Table
            expTable[i] = 0;
        }
    }
}

void rmvNode_clientsList(int left_node){//close fd and free clientnode
    clientNode * aux = clientsList_head;
    clientNode * free_aux = NULL;

    if(aux!= NULL){//if not empty
        if(aux->fd == left_node){//if left is the 1st node
            free_aux = aux;
            clientsList_head = aux->next;
            close(free_aux->fd);
            free(free_aux);
            return; // lrdy removed -- all done here
        }
    }

    while(aux->next != NULL){
        if(aux->next->fd == left_node){
            free_aux = aux->next;
            aux->next = aux->next->next;
            close(free_aux->fd);
            free(free_aux);
            return;
        }        
        aux=aux->next;
    }

}