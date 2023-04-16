#include <arpa/inet.h>		// ntohs()
#include <stdio.h>		// printf(), perror()
#include <stdlib.h>		// atoi()
#include <string.h>		// strlen()
#include <sys/socket.h>		// socket(), connect(), send(), recv()
#include <unistd.h>		// close()
#include <netdb.h>
#include <netinet/in.h>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <cstdio>

#include "PacketHeader.h"
static const int MAX_MESSAGE_SIZE = 256;

static const int WINDOWS = 3;

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
    socklen_t sock_len;
    sendto(sockfd, message, sizeof(message), MSG_NOSIGNAL, (const struct sockaddr *) &addr, sizeof(addr));
    printf("Start request sent.\n");
    char buf[1024] = { 0 };
    n = recvfrom(sockfd, (char *)buf, 1024,
             MSG_WAITALL, (struct sockaddr *) &addr, &sock_len);
    buf[n] = '\0';

    PacketHeader *ack = (PacketHeader*)buf;
//    printf("%d, %d, %d\n", ack->type, ack->seqNum, head.seqNum);
    if(ack->type == 3 && head.seqNum == ack->seqNum) {
        printf("Connection start!\n");

    }else{
        // (5) Close connection
        printf("Start failed.\n");
        close(sockfd);
    }








    int packet_length = 1472 - sizeof(head);

    std::ifstream is ("test.txt", std::ifstream::binary);

    // get length of file:
    is.seekg(0, is.end);
    int length = is.tellg();
    is.seekg(0, is.beg);
    int packets_num = length/packet_length+1;
    printf("%d\n",packets_num);
    char packets[packets_num][1500];

    char *buffer = new char[packet_length];

    // read data as a block:
    int flag = 0;
    while (is) {
        is.read(buffer, packet_length);
        strcpy(packets[flag],buffer);
        printf("%s\n",packets[flag])
        flag++;
    }
    buffer[is.gcount()] = '\0';
    strcpy(packets[flag],buffer);
//        printf("Final packet only have %ld\n", is.gcount());
    is.close();
    printf("Begin send");
    int seqNum = 0;
    for(int i=0;i<WINDOWS;i++){
        PacketHeader header;
        header.seqNum = seqNum;
        header.type = 2;
        header.length = sizeof(packets[seqNum]);
        char message[1800] = { 0 };
        memcpy(message, &header, sizeof(header));
        strcat(message,packets[seqNum]);
        sendto(sockfd, message, sizeof(message), MSG_NOSIGNAL, (const struct sockaddr *) &addr, sizeof(addr));
    }


    return 0;
}

int main(){
    send_start("127.0.0.1", 8080);
    return 0;
}
