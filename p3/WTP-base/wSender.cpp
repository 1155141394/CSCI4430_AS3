#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include "crc32.h"
#include "PacketHeader.h"
#include <iostream>
#include <fstream>
#include <random>
#include <deque>
#include "utilities.h"
using namespace std;

#define MAX_WTP_SIZE 1472
#define HEADER_LEN 16
#define START 0
#define FIN 1
#define DATA 2

/* class for generating sliding window and sequence for input file */
class slidingWindow{

    int cur_file_seq_num; // the newest sequence number in the buffer (latest read)
    int capacity; // max window size
    deque<WrappedBuffer> window_buf; // each element is a buffer with header and data
    ifstream file;

    /* push a new buffer to the end, if full, remove an old buffer in the front */
    void enqueue(WrappedBuffer buf){
        if (window_buf.size() == this->capacity){
            window_buf.pop_front();
        }
        window_buf.push_back(buf);
    }

public:
    slidingWindow(int capacity, char* filename){
        this->capacity = capacity;
        file.open(filename);
        file.seekg(0, file.beg);
        cur_file_seq_num = 0;
        //fill the window buffer
        while (cur_file_seq_num < capacity && this->file.tellg() != -1){
            forward_window();
        }
    }

    /* get buffer of index i in the current window */
    WrappedBuffer get_buffer(int i){
        if (i >= window_buf.size() || i < 0){
            cout << "invalid buffer index" << endl;
            exit(1);
        }
        return window_buf[i];
    }

    /* the seq num of the oldest buffer in the window (first to be sent) */
    int get_send_seq_num(){
        return (window_buf.size() > 0 ? window_buf.front().header.seqNum : -1);
    }

    int get_size(){
        return window_buf.size();
    }

    /* keep moving window forward until buffer with seq_num is the first,
     * same as forward_window in basic part, may be useful in bonus part */
    int move_window_to(int seq_num){
        while (get_send_seq_num() < seq_num){
            if (forward_window() == -1)
                return -1;
        }
        return 0;
    }

    /* move window 1 step forward */
    int forward_window(){
        //read 1 pieces of new data into buffer
        if (file.tellg() != -1){
            WrappedBuffer tmp;
            tmp.header.seqNum = cur_file_seq_num;
            file.read(tmp.data, WTP_DATA_SIZE);
            tmp.header.length = file.gcount();
            tmp.header.type = DATA;
            tmp.header.checksum = crc32(tmp.data, tmp.header.length);
            enqueue(tmp);
            cur_file_seq_num++;
            return 0;

        } else if (window_buf.size() != 0){
            window_buf.pop_front();
            return (window_buf.size() == 0 ? -1 : 0);
        }
        return -1;
    }

    void close_file(){
        file.close();
    }

};

/* for sending START and END */
void connector(int FLAG, int seqnum, int sockfd, sockaddr* addr_ptr, socklen_t* addrlen, logger &log){
    WrappedBuffer buffer, recv_buf;
    buffer.header.type = FLAG;
    buffer.header.seqNum = seqnum;
    buffer.header.length = 0;

    int first_time = 1;
    Timer timer, fin_timer;
    timer.startTimer();
    fin_timer.startTimer();
    while (true) {
        /* send data */
        if (timer.getTime() > 0.5 || first_time){
            print("sent", buffer.header);
            log.record(buffer.header);
            sendto(sockfd, &buffer, HEADER_LEN, MSG_NOSIGNAL, addr_ptr, *addrlen);
            timer.startTimer();
            first_time = 0;
        }
        /* check for ack */
        if(recvfrom(sockfd, &recv_buf, MAX_WTP_SIZE, MSG_DONTWAIT, addr_ptr, addrlen) > 0){
            print("received", recv_buf.header);
            log.record(recv_buf.header);
            if (recv_buf.header.type == 3 && recv_buf.header.seqNum == buffer.header.seqNum) {
                break;
            }
        } else {
            if (FLAG == FIN && fin_timer.getTime() > 40){
                break;
            }
        }
    }
}

/* for sending window */
void connector(int FLAG, slidingWindow &window, int sockfd, sockaddr* addr_ptr, socklen_t* addrlen, logger &log){
    WrappedBuffer buffer;

    int ack_count = 3, window_size = window.get_size(), cur_seq_num;
    Timer timer;
    timer.startTimer();
    while (true) {
        if (ack_count == 3 || timer.getTime() > 0.5){
            /* send all data in current window */
            cur_seq_num = window.get_send_seq_num();
            window_size = window.get_size(); //get current size of the window, may not be max window size
            for (int i = 0; i < window_size; i++) {
                WrappedBuffer tmp = window.get_buffer(i);
                print("sent", tmp.header);
                log.record(tmp.header);
                sendto(sockfd, &tmp, tmp.header.length + HEADER_LEN, MSG_NOSIGNAL, addr_ptr, *addrlen);
            }
            timer.startTimer();
            ack_count = 0;
        }
        /* look for ack */
        if(recvfrom(sockfd, &buffer, MAX_WTP_SIZE, MSG_DONTWAIT, addr_ptr, addrlen) > 0){
            print("received", buffer.header);
            log.record(buffer.header);
            if (buffer.header.type == 3){

                if (buffer.header.seqNum == cur_seq_num){
                    ack_count++; //dup ack
                } else if (buffer.header.seqNum > cur_seq_num && buffer.header.seqNum <= cur_seq_num + window_size){
                        if (window.move_window_to(buffer.header.seqNum) == -1)
                            break; //no more window
                    else {
                            ack_count = 3; //send next window
                    }
                }
            }
        }
    }
    window.close_file();
}


void sender(char* receiver_ip, int receiver_port, int window_size, char* input_file, char* log) {

    // creating a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(receiver_port);
    hostent *server = gethostbyname(receiver_ip);
    memcpy(&addr.sin_addr, server->h_addr, server->h_length);

    //random number generation (a bit confused whether it is random or set to 0)
    mt19937 mt(time(nullptr));
    int conn_num = mt();

    slidingWindow sliding_window(window_size, input_file);
    logger logfile(log);

    //send a START message and wait for ack
    connector(START, conn_num, sockfd, (sockaddr*)&addr, &addrlen, logfile);

    //send data
    connector(DATA, sliding_window, sockfd, (sockaddr*)&addr, &addrlen, logfile);

    //send a END message
    connector(FIN, conn_num, sockfd, (sockaddr*)&addr, &addrlen, logfile);

}

int main(int argc, const char **argv) {
    int ip_len = strlen(argv[1]);
    char *receiver_ip = new char[ip_len + 1];
    strcpy(receiver_ip, argv[1]);

    int receiver_port = atoi(argv[2]);

    int window_size = atoi(argv[3]);

    int input_file_len = strlen(argv[4]);
    char *input_file = new char[input_file_len + 1];
    strcpy(input_file, argv[4]);

    int log_len = strlen(argv[5]);
    char *log = new char[log_len + 1];
    strcpy(log, argv[5]);

    sender(receiver_ip, receiver_port, window_size, input_file, log);
}
