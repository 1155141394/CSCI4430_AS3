//
// Created by 陶智盛 on 15/4/2023.
//

#ifndef CSCI4430_AS3_PACKHEADER_H
#define CSCI4430_AS3_PACKHEADER_H

#ifndef __PACKET_HEADER_H__
#define __PACKET_HEADER_H__

struct PacketHeader
{
    unsigned int type;     // 0: START; 1: END; 2: DATA; 3: ACK
    unsigned int seqNum;   // Describe afterwards
    unsigned int length;   // Length of data; 0 for ACK packets
    unsigned int checksum; // 32-bit CRC
};

#endif

#endif //CSCI4430_AS3_PACKHEADER_H
