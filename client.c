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
#define HEADLEN 4

int getServerMessage(int sockfd){
    
    Header head;
    char* payload;
    int status = 0;
    int nbytes = 0;
    
    //printf("GET SERVER MESSAGE \n");
    if(read(sockfd,(Header *) &head, HEADLEN) == -1) {
        puts("read error!\n");
        return -2;
    }
    char type = head.type;
    int msgLen = head.length;

    switch(type) {
        case '3': //FWD
                    break;
        case '5': //NAK
                    printf("Sorry.\n");
                    status = 1;
                    break;
        case '6': //OFFLINE
                    printf("Someone is OFFLINE: ");
                    break;
        case '7': //ACK
                    printf("Welcome to chat room!\n");
                    break;
        case '8': //ONLINE
                    printf("Someone is ONLINE: ");
                    break;
        case '9': //IDLE
                    printf("Someone has been idle: ");
                    break;
        default: return -1;
    }

    if (msgLen == HEADLEN) {
        return status;
    }

    payload = malloc(msgLen - HEADLEN);
    if(read(sockfd, payload, msgLen - HEADLEN) == -1) {
        puts("read error!\n");
        return -2;
    }
    int p = 0;
    while(p < msgLen) {
        Attribute attr;
        memcpy((Attribute*)&attr, payload + p, HEADLEN);
        int subtype = attr.type;
        int subLen = attr.length;
        char* content = malloc(subLen - HEADLEN + 1);
        memcpy(content, payload + p + HEADLEN, subLen - HEADLEN);
        p += subLen;
        content[subLen - HEADLEN] = '\0';
        switch(subtype) {
            case 1:
                    printf("Failed for reason: %s\n", content);
                    break;
            case 2:
                    printf("%s\n", content);
                    break;
            case 3:
                    printf("Number of users online: %s\n", content);
                    break;
            case 4:
                    printf("-> %s\n", content);
                    break;
            default:
                    break;
        }
    }   
    free(payload);
    return status;
}

//Send a JOIN Message to server when connected to server
void sendJoin(int sockfd, char *arg[]){
    
    Header header;
    Attribute attr;

    int status = 0;
    int nameLen = strlen(arg[1]);
    header.vrsn = '3';
    header.type = '2';//JOIN

    attr.type = 2;//Username
    attr.length = nameLen + HEADLEN + 1;   //1 -> '\0'
    header.length = HEADLEN + attr.length;
    char* buf = malloc(header.length);
    memcpy(buf, (void*)&header, HEADLEN);
    memcpy(buf + HEADLEN, (void*)&attr, HEADLEN);
    memcpy(buf + HEADLEN + HEADLEN, (void*) arg[1], nameLen);
    buf[header.length - 1] = '\0';
    
    write(sockfd,(void *) buf, header.length);
    
    // Sleep to allow Server to reply
    sleep(1);
    status = getServerMessage(sockfd);
    if(status == 1){
        close(sockfd);
    }
    free(buf);
}

//Accept user input, and send it to server for broadcasting
void chat(int connectionDesc){
    
    Header head;
    Attribute clientAttribute;
    
    head.vrsn = '3';
    head.type = '4';
    int nread = 0;
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
        nread = (int)read(STDIN_FILENO, temp, sizeof(temp));
        if(nread > 0){
            temp[nread++] = '\0';
        }
        
        clientAttribute.type = 4;
        clientAttribute.length = HEADLEN + nread;
        head.length = HEADLEN + clientAttribute.length;
        char* buf = malloc(head.length);
        memcpy(buf, (Header*)&head, HEADLEN);
        memcpy(buf + HEADLEN, (Attribute*)&clientAttribute, HEADLEN);
        memcpy(buf + HEADLEN + HEADLEN, temp, nread);
        write(connectionDesc,(void *) buf, head.length);
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