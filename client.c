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
#include <sys/timerfd.h>
#define HEADLEN 4
#define MAXLINE 4096

int setupClientSocket(char *host, char* port) {  
    struct addrinfo addrCriteria;  
    memset(&addrCriteria, 0, sizeof(addrCriteria));  
    addrCriteria.ai_family = AF_UNSPEC;  
    addrCriteria.ai_socktype = SOCK_STREAM;  
    addrCriteria.ai_protocol = IPPROTO_TCP;  
      
    struct addrinfo *server_addr;   
    int retVal = getaddrinfo(host, port, &addrCriteria, &server_addr);  
    if(retVal != 0)  
        return -1;  
    int sock=-1;  
    struct addrinfo *addr = server_addr;  
    while(addr != NULL)  
    {    
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);  
        if(sock<0)  
            continue;  
        if(connect(sock, addr->ai_addr, addr->ai_addrlen) == 0)  
        {  
            break;  
        }  
        close(sock);  
        sock = -1;  
        addr = addr->ai_next;  
    }  
    freeaddrinfo(server_addr);  
    return sock;  
} 

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
    int msgLen = ntohs(head.length);
    switch(type) {
        case 2: //JOIN
            puts("join request from:");
            break;
        case 4: //SEND
            puts("send msg:");
            break;
        case 3: //FWD
            break;
        case 5: //NAK
            printf("Sorry. You are not allowed to join.\n");
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
        puts("null packet!\n");
        return status;
    }
    
    payload = calloc(1, msgLen - HEADLEN + 1);
    if(read(sockfd, payload, msgLen - HEADLEN) == -1) {
        puts("read error!\n");
        return -2;
    }
    int p = 0;
    // printf("payload : %d\n", msgLen - HEADLEN + 1);
    // printf("debug 1\n");
    while(p < msgLen - HEADLEN) {
        // printf("debug 2\n");
        Attribute attr;
        memcpy((Attribute*)&attr, payload + p, HEADLEN);
        // printf("debug 3\n");

        int subtype = ntohs(attr.type);
        int subLen = ntohs(attr.length);

        // printf("len: %d. ", subLen);
        // printf("debug 4\n");

        char* content = calloc(1, subLen - HEADLEN + 1);
        memcpy(content, payload + p + HEADLEN, subLen - HEADLEN);
        // printf("debug 5\n");
        Cnt *cnt = malloc(sizeof(Cnt));
        memcpy(cnt, content, sizeof(Cnt));
        int clientcount = ntohs(cnt->client);

        p += subLen;
        // content[subLen - HEADLEN] = '\0';
        switch(subtype) {
            case 1:
                printf("Failed for reason: %s\n", content);
                break;
            case 2:
                printf(" %s\n", content);
                break;
            case 3:
                printf("Number of users online: %d\n", clientcount);
                break;
            case 4:
                printf("-> %s\n", content);
                break;
            default:
                break;
        }
        // printf("debug 6\n");

        free(content);
        // printf("debug 7\n");

        // printf("pos: %d.\n", p);
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
    attr->type = htons(USERNAME); //Username
    header->length = htons(HEADLEN + HEADLEN + nameLen);
    attr->length = htons(nameLen + HEADLEN);
    
    void* buf = malloc(ntohs(header->length));
    memcpy(buf, header, HEADLEN);
    void* tmp = buf + HEADLEN;
    memcpy(tmp, attr, HEADLEN);
    tmp = tmp + HEADLEN;
    memcpy(tmp, (void*) user, nameLen);
    // printf("%d\n", ntohs(header->length));
    write(sockfd,(void *) buf, ntohs(header->length));
    
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
    header->length = htons(HEADLEN);
    
    void* buf = malloc(HEADLEN);
    memcpy(buf, header, HEADLEN);

    // printf("debug 7\n");
    write(sockfd,(void *) buf, HEADLEN);
    
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
    attr->type = htons(MESSAGE);

    int timerfd = timerfd_create(CLOCK_REALTIME, 0);
    struct itimerspec new_value, curr_value;
    uint64_t expTime;

    //timer
    new_value.it_value.tv_sec = 10;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;

    
    char temp[512];
    
    int maxfd, stdineof;
    fd_set rset;
    stdineof = 0;
    FD_ZERO(&rset);   
    if (timerfd_settime(timerfd, 0, &new_value, NULL) == -1)
        perror("timerfd_settime");

    for (;;) {
        if (stdineof == 0) FD_SET(fileno(fp), &rset);
        FD_SET(connectionDesc, &rset);
        FD_SET(timerfd, &rset);
        maxfd = fileno(fp) > connectionDesc ? fileno(fp) : connectionDesc;
        maxfd = (maxfd > timerfd ? maxfd : timerfd) + 1;
        select(maxfd, &rset, NULL, NULL, NULL);
        
        //be able to read from server
        if (FD_ISSET(connectionDesc, &rset)) {
        // printf("recieve mess\n");
            getServerMessage(connectionDesc);
        }

        //listen on keyboard available
        if (FD_ISSET(fileno(fp), &rset)) {
            if ((nread = (int)read(fileno(fp), temp, MAXLINE)) == 0) {
                stdineof = 1;
                shutdown(connectionDesc, SHUT_WR);
                FD_CLR(fileno(fp), &rset);
                continue;
            }

            //reset timer
            new_value.it_value.tv_sec = 10;
            if (timerfd_settime(timerfd, 0, &new_value, NULL) == -1)
                perror("timerfd_settime");

            nread = nread > 512 ? 512 : nread;
            attr->length = htons(HEADLEN + nread);
            int attrLenVal = ntohs(attr->length);
            header->length = htons(HEADLEN + attrLenVal);
            char* buf = malloc(ntohs(header->length));
            
            memcpy(buf, header, HEADLEN);
            void* tmp = buf + HEADLEN;
            memcpy(tmp, attr, HEADLEN);
            tmp = tmp + HEADLEN;
            memcpy(tmp, temp, nread);
            memset(temp, '\0', nread);
            write(connectionDesc,(void *) buf, ntohs(header->length));
            free(buf);
        } 

    //If timer out, set IDLE
        if(FD_ISSET(timerfd, &rset)){
        // printf("debug 1\n");
            read(timerfd, &expTime, sizeof(uint64_t));
            new_value.it_value.tv_sec = 0;
            if (timerfd_settime(timerfd, 0, &new_value, NULL) == -1)
                perror("timerfd_settime");
            sendIdle(connectionDesc);
        }
        
    }
    free(header);
    free(attr);
}

int main(int argc , char *argv[]){
    if (argc == 4){
        int socket_desc;
        // struct sockaddr_in server;
    
        // memset(&server, 0, sizeof(server));
        // server.sin_addr.s_addr = inet_addr(argv[2]);
        // server.sin_family = AF_INET;
        // server.sin_port = htons(atoi(argv[3]));
        
        //Create socket
        // socket_desc = socket(AF_INET , SOCK_STREAM , 0);
        socket_desc = setupClientSocket(argv[2], argv[3]); 
        if (socket_desc == -1){
            printf("Could not create socket");
            exit(0);
        }
        
        //Connect to remote server
        if (socket_desc < 0){
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
