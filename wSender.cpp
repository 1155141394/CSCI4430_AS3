#include <arpa/inet.h>		// ntohs()
#include <stdio.h>		// printf(), perror()
#include <stdlib.h>		// atoi()
#include <string.h>		// strlen()
#include <sys/socket.h>		// socket(), connect(), send(), recv()
#include <unistd.h>		// close()
#include <netdb.h>
#include <netinet/in.h>

#include "PacketHeader.h"
static const int MAX_MESSAGE_SIZE = 256;



int send_start(const char *hostname, int port) {

    PacketHeader head;
    head.type = 0;
    head.seqNum = rand();
    head.length = 0;
    int n;
    // 首先需要定义一个变量
    char message[1024] = { 0 };
    memcpy(message, &head, sizeof(head));

    if (strlen(message) > MAX_MESSAGE_SIZE) {
        perror("Error: Message exceeds maximum length\n");
        return -1;
    }

    // (1) Create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // (2) Create a sockaddr_in to specify remote host and port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    struct hostent *host = gethostbyname(hostname);
    if (host == nullptr) {
        fprintf(stderr, "%s: unknown host\n", hostname);
        return -1;
    }
    memcpy(&(addr.sin_addr), host->h_addr, host->h_length);
    addr.sin_port = htons(port);


    // (4) Send message to remote server
    // Call send() enough times to send all the data
    ssize_t message_len = strlen(message);
    socklen_t sock_len;
    sendto(sockfd, message, message_len, MSG_NOSIGNAL, (const struct sockaddr *) &addr, sizeof(addr));
    printf("Start request sent.\n");
    char buf[1024] = { 0 };
    n = recvfrom(sockfd, (char *)buf, 1024,
             MSG_WAITALL, (struct sockaddr *) &addr, &sock_len);
    buf[n] = '\0';

    PacketHeader *ack = (PacketHeader*)buf;
    printf("%d, %d, %d\n", ack->type, ack->seqNum, head.seqNum)
    if(ack->type == 3 && head.seqNum == ack->seqNum) {
        printf("Connection start!");
    }else{
        // (5) Close connection
        printf("Start failed.\n");
        close(sockfd);
    }


    // (5) Close connection
    close(sockfd);

    return 0;
}

int main(){
    send_start("127.0.0.1", 8080);
    return 0;
}
