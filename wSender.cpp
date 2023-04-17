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
#include <time.h>
#include <chrono>
#include<algorithm>
#include "crc32.h"
#include "PacketHeader.h"

using namespace std;
using namespace chrono;

static const int MAX_MESSAGE_SIZE = 256;



int logger(const char *filename,PacketHeader *head){
    FILE *fp = NULL;
    fp = fopen(filename, "a");
    fprintf(fp,"<%u><%u><%u><%u>\n",head->type,head->seqNum,head->length,head->checksum);
    fclose(fp);
    return 0;
}


int send_start(const char *hostname, int port,const char *input,const char *log,int size) {

    srand((unsigned)time(NULL));
    PacketHeader head;
    head.type = 0;
    head.seqNum = rand();
//    printf("%d\n",head.seqNum);
    head.length = 0;
    int n;

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
    logger(log,&head);

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

    std::ifstream is (input, std::ifstream::binary);

    // get length of file:
    is.seekg(0, is.end);
    int length = is.tellg();
    is.seekg(0, is.beg);
    int packets_num = length/packet_length+1;
    printf("packet_num is %d\n",packets_num);
    char packets[packets_num][1500];

    char *buffer = new char[packet_length];

    // read data as a block:
    int flag = 0;
    while (true) {
        is.read(buffer, packet_length);
        if(!is){
            break;
        }
        strcpy(packets[flag],buffer);
//        printf("%s\n",packets[flag]);
        flag++;
    }
    buffer[is.gcount()] = '\0';
    strcpy(packets[flag],buffer);
//    printf("Final packet only have %ld\n", is.gcount());
    is.close();
    printf("Begin sent.\n");

    int seqNum = 0;
    while(true){
        int sent_msg = 0;
        for(int i=0;i<size;i++){
            if(seqNum >= packets_num){
                break;
            }
            PacketHeader header;
            header.seqNum = seqNum;
            header.type = 2;
            header.length = strlen(packets[seqNum]);
            header.checksum = crc32(packets[seqNum],header.length);
            char message[1472] = {0};
            memcpy(message, &header, sizeof(header));

            for(int k = 16;k<1472;k++){
                message[k] = packets[seqNum][k-16];
            }
            //printf("%s\n",packets[seqNum]);
            sendto(sockfd, message, sizeof(message), MSG_DONTWAIT, (const struct sockaddr *) &addr, sizeof(addr));
            logger(log,&header);

            seqNum ++;
            sent_msg++;
            if(seqNum >= packets_num){
                break;
            }
        }

        auto start = system_clock::now();
        socklen_t len = sizeof(addr);
        char packet_ack[1024] = { 0 };
        int n = recvfrom(sockfd, (char *)packet_ack, 1024,
                         MSG_NOSIGNAL, ( struct sockaddr *) &addr, &len);
        packet_ack[n] = '\0';
        PacketHeader *ack_head = (PacketHeader*)packet_ack;
        seqNum =  ack_head->seqNum;
        logger(log,ack_head);
        auto end   = system_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        if(double(duration.count())>500){
            seqNum -= sent_msg;
            continue;
        }


//        int seq_list[WINDOWS]  = {0};
//        auto start = system_clock::now();
//        int flag = 0;
//        while(flag<sent_msg){
//            socklen_t len = sizeof(addr);
//            char packet_ack[1024] = { 0 };
//            int n = recvfrom(sockfd, (char *)packet_ack, 1024,
//                             MSG_NOSIGNAL, ( struct sockaddr *) &addr, &len);
//            packet_ack[n] = '\0';
//            PacketHeader *ack_head = (PacketHeader*)packet_ack;
//            if(ack_head->seqNum<=seqNum-sent_msg){
//                continue;
//            }
//
//            seq_list[flag] =  ack_head->seqNum;
//            flag ++;
//            auto end   = system_clock::now();
//            auto duration = duration_cast<microseconds>(end - start);
//            if(double(duration.count())>500){
//                break;
//            }
//        }
//
//
//        int maxValue = *max_element(seq_list,seq_list+WINDOWS);
//        if(maxValue == 0){
//            maxValue = seqNum;
//
//        }
//        printf("seqNUm %d\n", maxValue);
//        seqNum = maxValue;
//        printf("seq is %d\n",seqNum);
        if(seqNum >= packets_num){
            break;
        }

    }
    head.type = 1;
    char end[1024] = { 0 };
    memcpy(end, &head, sizeof(head));
    sendto(sockfd, end, sizeof(end), MSG_NOSIGNAL, (const struct sockaddr *) &addr, sizeof(addr));
    logger(log,&head);
    char end_ack[1024] = { 0 };
    n = recvfrom(sockfd, (char *)end_ack, 1024,
                 MSG_WAITALL, (struct sockaddr *) &addr, &sock_len);
    end_ack[n] = '\0';

    PacketHeader *ack_message = (PacketHeader*)end_ack;
//    printf("%d, %d, %d\n", ack->type, ack->seqNum, head.seqNum);
    if(ack_message->type == 3 && head.seqNum == ack_message->seqNum) {
        printf("Connection end!\n");

    }else{
        // (5) Close connection
        printf("End failed.\n");
    }
    close(sockfd);



    return 0;
}

int main(int argc, const char **argv){
    const char *hostname = argv[1];
    int port = atoi(argv[2]);
    int size = atoi(argv[3]);
    const char *input = argv[4];
    const char *log = argv[5];

//    send_start(hostname, port);
    send_start(hostname,port, input,log, size);
    return 0;
}
