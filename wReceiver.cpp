#include <arpa/inet.h>		// ntohs()
#include <stdio.h>		// printf(), perror()
#include <stdlib.h>		// atoi()
#include <string.h>		// strlen()
#include <sys/socket.h>		// socket(), connect(), send(), recv()
#include <unistd.h>		// close()
#include <netdb.h>
#include <netinet/in.h>
#include <cstdio>
#include <fstream>
#include "crc32.h"
#include "PacketHeader.h"

#define MAXSIZE 1472

int get_port_number(int sockfd) {
    struct sockaddr_in addr;
    socklen_t length = sizeof(addr);
    if (getsockname(sockfd, (sockaddr *) &addr, &length) == -1) {
        perror("Error getting port of socket");
        return -1;
    }
    // Use ntohs to convert from network byte order to host byte order.
    return ntohs(addr.sin_port);
}


int run_server(int port, int queue_size, char * store_dir) {

    // (1) Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("Error opening stream socket");
        return -1;
    }

    // (2) Set the "reuse port" socket option
    int yesval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
        perror("Error setting socket options");
        return -1;
    }

    // (3) Create a sockaddr_in struct for the proper port and bind() to it.
    struct sockaddr_in servaddr, cliaddr;

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    // (3b) Bind to the port.
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("Error binding stream socket");
        return -1;
    }
    socklen_t len;
    int n;
    len = sizeof(cliaddr);

    // receive datagram from client

    char msg[1024] = { 0 };
    n = recvfrom(sockfd, (char *)msg, 1024,
                 MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
    msg[n] = '\0';
    printf("Receive things back from sender.\n");
    printf("%s\n",msg);
    PacketHeader *head = (PacketHeader*)msg;
    int header_len = sizeof(*head);
    if(head->type == 0){
        head->type = 3;
//        printf("%d\n",head->seqNum);
        // 首先需要定义一个变量
        char ack[1024] = { 0 };
        memcpy(ack, head, sizeof(*head));
        sendto(sockfd, ack, sizeof(ack), MSG_NOSIGNAL, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
        printf("ack back!\n");
    }

//     Start receive data
    PacketHeader ack_header;
    char ack[1024] = { 0 };
    ack_header.type = 3;
    ack_header.length = 0;
    char data[2000] = {0};
    char recv_header_msg[100] = {0};
    int seq_num = -1;
    unsigned int checksum = 0;
    while(1){
        memset(data, 0, 2000);
        printf("Start for loop.\n");
        // receive data from sender
        memset(msg, 0, 1024);
        n = recvfrom(sockfd, (char *)msg, MAXSIZE,
                     MSG_NOSIGNAL, ( struct sockaddr *) &cliaddr, &len);
        msg[n] = '\0';
        printf("Received message lenght: %d\n", n);
        // Get out the header
        printf("Header length: %d\n", header_len);
        for(int i = 0; i < header_len; i++){
            recv_header_msg[i] = msg[i];
        }
        PacketHeader *recv_header = (PacketHeader*)recv_header_msg;
        int len = recv_header->length;
        printf("Data length: %d\n", len);
        printf("Current seq_num: %d, Received seq_num: %d\n", seq_num, recv_header->seqNum);
        if (recv_header->seqNum == seq_num + 1) {
            printf("Received package successfully.\n");
            seq_num = recv_header->seqNum;

            // get out the data part
            for(int i = 0; i < len; i++){
                data[i] = msg[header_len + i];
            }
//            printf("Data: %s\n", data);
            checksum = crc32(data, len);
            if (checksum != recv_header -> checksum) {
                continue;
            }
            ack_header.seqNum = seq_num + 1;
            memcpy(ack, &ack_header, sizeof(*head));
            sendto(sockfd, ack, sizeof(ack), MSG_NOSIGNAL, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));

            // store the data in a txt file
            std::ofstream stream;
            stream.open(store_dir, std::ios_base::app); // open file stream
            if( !stream )
                std::cout << "Opening file failed" << endl;
            stream << testDest; // write char * into file stream
            if( !stream )
                cout << "Write failed" << endl;

        }
        else {
            printf("Not received package\n");
            continue;
        }

    }

    return 0;
}

int main(){
    run_server(8080, 10, "./data.txt");
    return 0;
}
