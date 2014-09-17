#include "client.h"
#include <iostream>
using namespace std;


Client::Client(char *hName)
{
    strcpy(this->hostName, hName); 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
}

int Client::connectToHost()
{
    struct addrinfo *serverInfo;
    char s[INET6_ADDRSTRLEN];
    int rVal = getaddrinfo(this->hostName, PORT, &this->hints, &serverInfo);
    if (rVal != 0)
    {
        // Non-zero return value indicates an error
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rVal));
        return -1;
    }

    /*
     * Loop through results and connect to the first we can
     */
    struct addrinfo *temp;
    for (temp = serverInfo; temp != NULL; temp = temp->ai_next)
    {
        sockFd = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
        if (sockFd == -1)
        {
            perror("client: Unable to create socket");
            continue;
        }

        if (connect(sockFd, temp->ai_addr, temp->ai_addrlen) == -1)
        {
            close(sockFd);
            perror("client: Error in connecting");
            continue;
        }

        break;
    }

    if (temp == NULL)
    {
        fprintf(stderr, "client: Failed to connect\n");
        return -2;
    }

    inet_ntop(temp->ai_family, get_in_addr((struct sockaddr*)temp->ai_addr), s, sizeof s);
    printf("client: Connecting to %s\n", s);

    freeaddrinfo(serverInfo);
    return 0;
}

int Client::recvData()
{
    int numBytes;
    char buf[MAXDATASIZE];
    numBytes = recv(sockFd, buf, MAXDATASIZE-1, 0);
    if (numBytes == -1)
    {
        perror("Error in receiving data from the server");
        exit(1);
    }
    buf[numBytes] = '\0';
    printf("Client: Received '%s' from the server \n", buf);
    close(sockFd);
    return numBytes;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: client <hostname>\n");
        exit(1);
    }
    Client *c = new Client(argv[1]);

    int retVal = c->connectToHost();
    if (retVal < 0)
    {
        fprintf(stderr, "Connection failed\n");
        exit(1);
    }
    c->recvData();
    return 0;
}
