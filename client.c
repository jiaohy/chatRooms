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
#define MAXLINE 4096

int getServerMessage(int sockfd){
    
    Header head;
    char* payload;
    int status = 0;
    
    //printf("GET SERVER MESSAGE \n");
    if(read(sockfd,(Header *) &head, HEADLEN) == -1) {
        puts("read error!\n");
        return -2;
    }
    int type = ((int)head.type & 0x7f);
    int msgLen = head.length;
    
    switch(type) {
        case 3: //FWD
            break;
        case 5: //NAK
            printf("Sorry.\n");
            status = 1;
            break;
        case 6: //OFFLINE
            printf("Someone is OFFLINE: ");
            break;
        case 7: //ACK
            printf("Welcome to chat room!\n");
            break;
        case 8: //ONLINE
            printf("Someone is ONLINE: ");
            break;
        case 9: //IDLE
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
        free(content);
    }
    free(payload);
    return status;
}

void sendJoin(int sockfd, char *user){
    
    Header *header;
    Attribute *attr;
    
    int status = 0;
    int nameLen = (int)strlen(user) + 1;
    int version = 3;
    int sbcp_type = JOIN;
    
    header = malloc(HEADLEN);
    header->vrsn = version>>1;
    header->type = (((version&0x1)<<7)|(sbcp_type&0x7f));
    
    attr = malloc(HEADLEN);
    attr->type = USERNAME;//Username
    attr->length = nameLen + HEADLEN;
    header->length = HEADLEN + attr->length;
    
    void* buf = malloc(header->length);
    memcpy(buf, header, HEADLEN);
    void* tmp = buf + HEADLEN;
    memcpy(tmp, attr, HEADLEN);
    tmp = tmp + HEADLEN;
    memcpy(tmp, (void*) user, nameLen);

    write(sockfd,(void *) buf, header->length);
    
    // Sleep to allow Server to reply
    sleep(1);
    status = getServerMessage(sockfd);
    if(status == 1){
        close(sockfd);
    }
    free(buf);
    free(header);
    free(attr);
}

void sendIdle(int sockfd) {
    Header *header;
    
    int version = 3;
    int sbcp_type = IDLE;
    
    header = malloc(HEADLEN);
    header->vrsn = version>>1;
    header->type = (((version&0x1)<<7)|(sbcp_type&0x7f));
    header->length = HEADLEN;
    
    void* buf = malloc(HEADLEN);
    memcpy(buf, header, HEADLEN);

    
    write(sockfd,(void *) buf, header->length);
    
    free(buf);
    free(header);
}

//Accept user input, and send it to server for broadcasting
void chat(FILE *fp, int connectionDesc){
    
    Header *header;
    Attribute *attr;
    int nread = 0;
    int version = 3;
    int sbcp_type = SEND;
    
    header = malloc(HEADLEN);
    header->vrsn = version>>1;
    header->type = (((version&0x1)<<7)|(sbcp_type&0x7f));
    
    attr = malloc(HEADLEN);
    attr->type = MESSAGE;
    
    char temp[512];
    
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    int maxfd, stdineof;
    fd_set rset;
    FD_ZERO(&rset);
    stdineof = 0;

    for (;;) {
        if (stdineof == 0) FD_SET(fileno(fp), &rset);
        FD_SET(connectionDesc, &rset);
        maxfd = (fileno(fp) > connectionDesc ? fileno(fp) : connectionDesc) + 1;
        select(maxfd, &rset, NULL, NULL, &tv);
        
        if (FD_ISSET(connectionDesc, &rset)) {
            getServerMessage(connectionDesc);
        }
        
        if (FD_ISSET(fileno(fp), &rset)) {
            if ((nread = (int)read(fileno(fp), temp, MAXLINE)) == 0) {
                stdineof = 1;
                shutdown(connectionDesc, SHUT_WR);
                FD_CLR(fileno(fp), &rset);
                continue;
            }
            attr->length = HEADLEN + nread;
            header->length = HEADLEN + attr->length;
            char* buf = malloc(header->length);
            
            memcpy(buf, header, HEADLEN);
            void* tmp = buf + HEADLEN;
            memcpy(tmp, attr, HEADLEN);
            tmp = tmp + HEADLEN;
            memcpy(tmp, temp, nread + 1);
            memset(temp, '\0', nread + 1);
            
            write(connectionDesc,(void *) buf, header->length);
            free(buf);
        } else {
            sendIdle(connectionDesc);
            printf("Timed out.\n");
        }
    }
    free(header);
    free(attr);
}

int main(int argc , char *argv[]){
    if (argc == 4){
        int socket_desc;
        struct sockaddr_in server;
    
    
        memset(&server, 0, sizeof(server));
        server.sin_addr.s_addr = inet_addr(argv[2]);
        server.sin_family = AF_INET;
        server.sin_port = htons(atoi(argv[3]));
        
        //Create socket
        socket_desc = socket(AF_INET , SOCK_STREAM , 0);
        if (socket_desc == -1){
            printf("Could not create socket");
            exit(0);
        }
        
        //Connect to remote server
        if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0){
            puts("connect error");
            exit(0);
        } else {
            puts("Connected\n");
            sendJoin(socket_desc, argv[1]);
            chat(stdin, socket_desc);
        }
    } else {
        puts("\n PARAMETER ERROR!");
    }
    puts("\n Client close \n");
    return 0;
}