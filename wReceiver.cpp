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
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    // (3b) Bind to the port.
    if (bind(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("Error binding stream socket");
        return -1;
    }

    // (3c) Detect which port was chosen.
    port = get_port_number(sockfd);
    printf("Server listening on port %d...\n", port);

    // (4) Begin listening for incoming connections.
    listen(sockfd, queue_size);

    // (5) Serve incoming connections one by one forever.
    while (true) {
        int connectionfd = accept(sockfd, 0, 0);
        if (connectionfd == -1) {
            perror("Error accepting connection");
            return -1;
        }

        // (1) Receive message from client.

        while(1){
            char msg[1024] = { 0 };
            int rev = recv(connectionfd, msg, strlen(msg), 0);
            if(rev<=0){
                close(connectfd);
                break;
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
        }

        // (4) Close connection
        close(connectionfd);
    }
}
