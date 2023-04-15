#include <arpa/inet.h>		// ntohs()
#include <stdio.h>		// printf(), perror()
#include <stdlib.h>		// atoi()
#include <string.h>		// strlen()
#include <sys/socket.h>		// socket(), connect(), send(), recv()
#include <unistd.h>		// close()
#include <netdb.h>
#include <netinet/in.h>

#include 'PacketHeader.h'
static const int MAX_MESSAGE_SIZE = 256;



int send_start(const char *hostname, int port) {

    PacketHeader head;
    head.type = 0;
    head.seqNum = rand();
    head.length = 0;

    // 首先需要定义一个变量
    char message[1024] = { 0 };
    memcpy(message, &head, sizeof(head));

    if (strlen(message) > MAX_MESSAGE_SIZE) {
        perror("Error: Message exceeds maximum length\n");
        return -1;
    }

    // (1) Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // (2) Create a sockaddr_in to specify remote host and port
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    struct hostent *host = gethostbyname(hostname);
    if (host == nullptr) {
        fprintf(stderr, "%s: unknown host\n", hostname);
        return -1;
    }
    memcpy(&(addr.sin_addr), host->h_addr, host->h_length);
    addr.sin_port = htons(port);

    // (3) Connect to remote server
    if (connect(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("Error connecting stream socket");
        return -1;
    }

    // (4) Send message to remote server
    // Call send() enough times to send all the data
    ssize_t message_len = strlen(message);
    send(sockfd, message, message_len, 0);

    char buf[1024] = { 0 };
    recv(sockfd,buf,strlen(buf),0);
    PacketHeader *ack = (PacketHeader*)buf;
    if(ack.type == 3 && head.seqNum == ack->seqNum) {
        printf("Connection start!");
    }else{
        // (5) Close connection
        close(sockfd);
    }


    // (5) Close connection
    close(sockfd);

    return 0;
}
