#include <arpa/inet.h>		// ntohs()
#include <stdio.h>		// printf(), perror()
#include <stdlib.h>		// atoi()
#include <string.h>		// strlen()
#include <sys/socket.h>		// socket(), connect(), send(), recv()
#include <unistd.h>		// close()
#include <netdb.h>
#include <netinet/in.h>

#include 'PacketHeader.h'

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


int run_server(int port, int queue_size) {

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
    if(rev<=0){
        close(connectfd);
        printf("Server failed.\n");
    }
    PacketHeader *head = (PacketHeader*)msg;
    if(head->type == 0){
        head->type = 3;
        // 首先需要定义一个变量
        char ack[1024] = { 0 };
        memcpy(ack, &head, sizeof(head));
        send(connectionfd,ack,strlen(ack),0);
        printf("ack back!");
    }
    return 0;
}
