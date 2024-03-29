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
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include<cstdio>
#include "../starter_files/crc32.h"
#include "../starter_files/PacketHeader.h"

#define MAXSIZE 1472

using namespace std;

int logger(const char *filename,PacketHeader *head){
    FILE *fp = NULL;
    fp = fopen(filename, "a");
    fprintf(fp,"%u %u %u %u\n",head->type,head->seqNum,head->length,head->checksum);
    fclose(fp);
    return 0;
}

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



int run_server(int port, int queue_size, int window_size, char * store_dir, const char * log_dir) {

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
//    char all_store_file_dir[30] = {0};
//    sprintf(all_store_file_dir, "%s/FILE-%d.out", store_dir, 0);
//    remove(all_store_file_dir);
    int count = 0;
    while (1) {

        char store_file_dir[30] = {0};
        sprintf(store_file_dir, "%s/FILE-%d.out", store_dir, count);

        // receive datagram from client

        char msg[2000] = { 0 };
        n = recvfrom(sockfd, (char *)msg, 1024,
                     MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        msg[n] = '\0';

        PacketHeader *head = (PacketHeader*)msg;
        logger(log_dir, head);
        int header_len = sizeof(*head);
        if(head->type == 0){
            head->type = 3;
//        printf("%d\n",head->seqNum);
            // 首先需要定义一个变量
            char ack[1024] = { 0 };
            logger(log_dir, head);
            memcpy(ack, head, sizeof(*head));
            sendto(sockfd, ack, sizeof(ack), MSG_NOSIGNAL, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
//            printf("ack back!\n");
        }

//     Start receive data
        PacketHeader ack_header;
        char ack[1024] = { 0 };
        ack_header.type = 3;
        ack_header.length = 0;
        char data[2000][1500] = {0};
        int data_len[2000] = {0};
        char recv_header_msg[100] = {0};
        int seq_num = -1;
        unsigned int checksum = 0;
        unsigned int end_seq = -1;
        int count = 0;
        while(1){
//            memset(data, 0, 2000);
            memset(msg, 0, 2000);

            n = recvfrom(sockfd, (char *)msg, MAXSIZE,
                         MSG_DONTWAIT, ( struct sockaddr *) &cliaddr, &len);
            if (n == -1) {
                continue;
            }
            else {
                msg[n] = '\0';
//                    printf("Received message lenght: %d\n", n);
                // Get out the header
//                    printf("Header length: %d\n", header_len);
                for(int i = 0; i < header_len; i++){
                    recv_header_msg[i] = msg[i];
                }
                PacketHeader *recv_header = (PacketHeader*)recv_header_msg;
                int len = recv_header->length;
//                    printf("Data length: %d\n", len);
//                    printf("Current seq_num: %d, Received seq_num: %d\n", seq_num, recv_header->seqNum);
                logger(log_dir, recv_header);
                // check if the connection is end

                if (recv_header->type == 1) {
                    end_seq = recv_header -> seqNum;
                    ack_header.seqNum = end_seq;
                    logger(log_dir, &ack_header);
                    memcpy(ack, &ack_header, sizeof(*head));
                    sendto(sockfd, ack, sizeof(ack), MSG_NOSIGNAL, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
                    break;
                }
                else if (recv_header->type == 0) {
                    ack_header.seqNum = recv_header->seqNum;
                    logger(log_dir, &ack_header);
                    memcpy(ack, &ack_header, sizeof(*head));
                    sendto(sockfd, ack, sizeof(ack), MSG_NOSIGNAL, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
                }
                else if ((int(recv_header->seqNum) <= seq_num)) {
                    ack_header.seqNum = recv_header->seqNum;
                    logger(log_dir, &ack_header);
                    memcpy(ack, &ack_header, sizeof(*head));
                    sendto(sockfd, ack, sizeof(ack), MSG_NOSIGNAL, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
                }
                else if ((recv_header->type == 2) and (int(recv_header->seqNum) > seq_num) and (int(recv_header->seqNum) <= (seq_num + window_size))) {
//                    printf("%d, %d, %d\n", recv_header->seqNum, recv_header->type, recv_header->length);
                    for(int i = 0; i < len; i++){
                        data[recv_header->seqNum][i] = msg[header_len + i];
                    }
                    data[recv_header->seqNum][len] = '\0';

                    checksum = crc32(data[recv_header->seqNum], len);
                    if (checksum != recv_header -> checksum) {
                        memset(data[recv_header->seqNum], 0, 2000);
                        continue;
                    }

                    else {
                        count += 1;
                        data_len[recv_header->seqNum] = len;
                        ack_header.seqNum = recv_header->seqNum;
                        logger(log_dir, &ack_header);
                        memcpy(ack, &ack_header, sizeof(*head));
                        sendto(sockfd, ack, sizeof(ack), MSG_NOSIGNAL, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
                        if (count == window_size) {
                            seq_num += window_size;
                            count = 0;
                        }
                    }
                }
                else {
                    continue;
                }
            }


            // The connection is not end and ask for new packages

                // The connection is finished

        }
        ofstream stream;
        stream.open(store_file_dir); // open file stream
        if( !stream )
            cout << "Opening file failed" << endl;
//                        printf("%ld, %d\n", strlen(data), seq_num);
        for (int i = 0; i <= seq_num + count;i++) {
            stream.write(data[i], data_len[i]); // write char * into file stream
        }
        if( !stream )
            cout << "Write failed" << endl;
        stream.close();
        count += 1;
    }
    return 0;
}

int main(int argc, char** argv){
    int listen_port = atoi(argv[1]);
    int window_size = atoi(argv[2]);
    const char * log_dir = argv[4];
    char * store_dir = argv[3];
    char store_file_dir[30] = {0};

//    if (mkdir(store_dir, 0777) == -1)
//        cerr << "Error \n:  " << strerror(errno) << endl;
//
//    else
//        cout << "Directory created\n";


    for (int i = 0; i < strlen(store_dir); i++) {
        store_file_dir[i] = store_dir[i];
    }
//    strcat(store_file_dir, "/FILE-i.out");
    printf("%s\n", store_file_dir);
    run_server(listen_port, 10, window_size, store_file_dir, log_dir);
    return 0;
}
