#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h> //inet_addr
#include"SBCP.h"

int getServerMessage(int sockfd){
    
    Msg serverMessage;
    int status = 0;
    long nbytes = 0;
    
    //printf("GET SERVER MESSAGE \n");
    nbytes=read(sockfd,(Msg *) &serverMessage,sizeof(serverMessage));
    
    if(serverMessage.header.type == 3){
        if(serverMessage.attribute.type == 4){
            printf("Forwarded Message from user %s:", serverMessage.attribute.payload);
        }
        if (serverMessage.attribute.type == 2){
            printf("%s \n", serverMessage.attribute.payload);
        }
        if (serverMessage.attribute.type == 3){
            printf("%s users are now online. \n", serverMessage.attribute.payload);
        }
        status=0;
    }
    
    if(serverMessage.header.type==5){
        if(serverMessage.attribute.type==1){
            printf("NAK Message from Server: %s \n",serverMessage.attribute.payload);
        }
        status=1;
    }
    
    if(serverMessage.header.type==6){
        if(serverMessage.attribute.type==2){
            printf("%s is now OFFLINE \n",serverMessage.attribute.payload);
        }
        status=0;
    }
    
    if(serverMessage.header.type==7){
        if(serverMessage.attribute.type==4){
            printf("ACK Message from Server is %s",serverMessage.attribute.payload);
        }
        status=0;
    }
    
    if(serverMessage.header.type==8){
        if(serverMessage.attribute.type==2){
            printf("%s is now ONLINE",serverMessage.attribute.payload);
        }
        status=0;
    }
    
    if(serverMessage.header.type==9){
        if(serverMessage.attribute.type==2){
            printf("%s is now IDLE",serverMessage.attribute.payload);
        }
        status=0;
    }
    
    return status;
}

//Send a JOIN Message to server when connected to server
void sendJoin(int sockfd, char *arg[]){
    
    Header header;
    Attribute attrib;
    Msg msg;
    int status = 0;
    
    header.vrsn = '3';
    header.type = '2';//JOIN

    attrib.type = 2;//Username
    attrib.length = (int)strlen(arg[1]) + 1;
    strcpy(attrib.payload,arg[1]);
    msg.header = header;
    msg.attribute = attrib;
    
    write(sockfd,(void *) &msg,sizeof(msg));
    
    // Sleep to allow Server to reply
    sleep(1);
    status = getServerMessage(sockfd);
    if(status == 1){
        close(sockfd);
    }
}

//Accept user input, and send it to server for broadcasting
void chat(int connectionDesc){
    
    Msg msg;
    Attribute clientAttribute;
    
    long nread = 0;
    char temp[512];

    struct timeval tv;
    fd_set readfds;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    // don't care about writefds and exceptfds:
    select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(STDIN_FILENO, &readfds)){
        nread = read(STDIN_FILENO, temp, sizeof(temp));
        if(nread > 0){
            temp[nread] = '\0';
        }
        
        clientAttribute.type = 4;
        strcpy(clientAttribute.payload,temp);
        msg.attribute = clientAttribute;
        write(connectionDesc,(void *) &msg,sizeof(msg));
    } else {
        printf("Timed out.\n");
    }
}

int main(int argc , char *argv[]){
    if (argc == 4){
        int socket_desc;
        struct sockaddr_in server;

        fd_set master;
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&master);
    
        //Create socket
        socket_desc = socket(AF_INET , SOCK_STREAM , 0);
        if (socket_desc == -1){
            printf("Could not create socket");
            exit(0);
        } else {
            printf("Socket successfully created.\n");
        }
    
        bzero(&server,sizeof(server));
        server.sin_addr.s_addr = inet_addr(argv[2]);
        server.sin_family = AF_INET;
        server.sin_port = htons(atoi(argv[3]));
    
        //Connect to remote server
        if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0){
            puts("connect error");
            exit(0);
        } else {
            puts("Connected\n");
            sendJoin(socket_desc, argv);
            FD_SET(socket_desc, &master);
            FD_SET(STDIN_FILENO, &master);
            
            for (; ; ) {
                read_fds = master;
                puts("\n");
                if (select(socket_desc + 1, &read_fds, NULL, NULL, NULL) == -1) {
                    perror("select");
                    exit(4);
                }
                if (FD_ISSET(socket_desc, &read_fds)) {
                    getServerMessage(socket_desc);
                }
                if (FD_ISSET(socket_desc, &read_fds)) {
                    chat(socket_desc);
                }
            }
        }
    } else {
        puts("\n PARAMETER ERROR!");
    }
    puts("\n Client close \n");
    return 0;
}