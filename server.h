#ifndef __SERVER__H__
#define __SERVER__H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"
#define BACKLOG 10

class Server
{
    private:
        struct addrinfo hints;
        int sockFd;
        int newConnFd;

    public:
        Server();
        int createSocketAndBind();
        int listenForConnections();
        int acceptConnection();
        int sendData(int sockFd, void * buf, size_t len, int flags = 0);
        int recvData(int sockFd);
};

#endif /* __SERVER__H__ */
