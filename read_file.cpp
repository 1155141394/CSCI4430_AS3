// read a file into memory
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <cstdio>

#include "PacketHeader.h"
int main () {
    PacketHeader head;
    int packet_length = 1472 - sizeof(head);
    printf("packet length is %d\n",packet_length);
    std::ifstream is ("test.txt", std::ifstream::binary);
    if (is) {
        // get length of file:
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);
        printf("length is %d\n",length);
        char *buffer = new char[packet_length];

        // read data as a block:
        is.read(buffer, packet_length);
//        printf("%s\n", buffer);
        while (is) {
            printf("read successfully.\n");
//            printf("%s\n", buffer);
            is.read(buffer, packet_length);
        }
        buffer[is.gcount()] = '\0';
        printf("%s\n", buffer);
        printf("Final packet only have %ld\n", is.gcount());
        is.close();
    }
    return 0;
}