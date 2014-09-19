#include "client.h"
#include <iostream>
using namespace std;

Client::Client(char *usrName, char *hName)
{
    isConnected = false;
    strcpy(this->userName, usrName);
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
    isConnected = true;

    freeaddrinfo(serverInfo);
    return 0;
}

int Client::recvData()
{
    int numBytes;
    SBMPHeaderT *recvHeader = new SBMPHeaderT();
    numBytes = recv(sockFd, recvHeader, sizeof(SBMPHeaderT), 0);
    if (numBytes == -1)
    {
        perror("Error in receiving data from the server");
        exit(1);
    }

    cout << " Received message from server : " << recvHeader->attributes[0].payload.message << endl;
    //close(sockFd);
    return numBytes;
}

int Client::sendData(void *msg, size_t len, int flags)
{
    int retVal;
    retVal = send(sockFd, msg, len, flags);
    return retVal;
}

bool Client::getConnectionStatus()
{
    return isConnected;
}

SBMPHeaderT* Client::createMessagePacket(SBMPMessageTypeT msgType, char *msg)
{
    SBMPAttributeT sbmpAttr;
    SBMPHeaderT *sbmpHeader;

    switch(msgType)
    {
        case JOIN:

            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_USER;
            strcpy(sbmpAttr.payload.username, this->userName);
            sbmpAttr.length = strlen(sbmpAttr.payload.username) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)JOIN;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;
        
        case FWD:
            break;

        case SEND:

            /*
             * Fill in the SBMP Attribute struct
             */   

            sbmpAttr.type = ATTR_MSG;
            strcpy(sbmpAttr.payload.message, msg);
            sbmpAttr.length = strlen(sbmpAttr.payload.message) + 4;

            /*
             * Fill in the SBMP Header struct
             */

            sbmpHeader = new SBMPHeaderT();
            sbmpHeader->version = 1;
            sbmpHeader->type = (int)SEND;
            sbmpHeader->length = sbmpAttr.length + 4;
            sbmpHeader->attributes[0] = sbmpAttr;

            break;

        default:
            printf("Error!!!Did not match any known message types. Exiting");
            exit(1);
    }
    return sbmpHeader;
}

int Client::getSocketFd()
{
    return sockFd;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: client <username> <hostname>\n");
        exit(1);
    }
    Client *c = new Client(argv[1], argv[2]);

    int retVal = c->connectToHost();
    if (retVal < 0)
    {
        fprintf(stderr, "Connection failed\n");
        exit(1);
    }

    if (c->getConnectionStatus())
    {
        /*
         * Send JOIN message to the server
         */
        SBMPHeaderT * sbmpHeader = c->createMessagePacket(JOIN, NULL);
        c->sendData(sbmpHeader, sizeof(SBMPHeader));

    }

    int clientFd = c->getSocketFd();
    fd_set read_fds;
    int fdMax;
    FD_ZERO(&read_fds);
    fdMax = clientFd;
    FD_SET(c->getSocketFd(), &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);


    char message[512];
    while(1)
    {
        if (select (fdMax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("Select");
            exit(4);
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            /*
             * Read input from user
             */
            //fgets(message, 512, STDIN_FILENO);
            cin.getline(message, 512);
            SBMPHeaderT * sbmpHeader = c->createMessagePacket(SEND, message);
            c->sendData(sbmpHeader, sizeof(SBMPHeader));
            fflush(STDIN_FILENO);
        }

        if (FD_ISSET(clientFd, &read_fds))
        {
            /*
             * Get broadcast message from server
             */
            c->recvData();
            fflush(STDIN_FILENO);
        }
    }

    return 0;
}
