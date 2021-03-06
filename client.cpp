#include "client.h"
#include <iostream>
using namespace std;

Client::Client(char *usrName, char *hName, char *servPort)
{
    isConnected = false;
    strcpy(this->userName, usrName);
    strcpy(this->hostName, hName);
    strcpy(this->serverPort, servPort);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
}

int Client::connectToHost()
{
    struct addrinfo *serverInfo;
    char s[INET6_ADDRSTRLEN];
    int rVal = getaddrinfo(this->hostName, this->serverPort, &this->hints, &serverInfo);
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
    printf("Establishing connecting with the server at %s\n", s);
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
    else if (numBytes == 0)
    {
        cout << "Server has closed connection. Exiting!" << endl;
        exit(2);
    }

    SBMPMessageTypeT msgType = (SBMPMessageTypeT) recvHeader->type; 
    switch (msgType)
    {
        case FWD:
            {
                string userName(recvHeader->attributes[0].payload.username);
                string msg(recvHeader->attributes[1].payload.message);
                cout << endl << userName << " : " << msg << endl;
            }
            break;

        case SEND:
            break;

        case JOIN:
            break;

        case ACK:
            {
                string msg(recvHeader->attributes[0].payload.message);
                char *ackMsg = new char[msg.size() + 1];
                std::copy(msg.begin(), msg.end(), ackMsg);

                char * p = strtok(ackMsg, " ");
                int n = 1; 
                printf ("\nCURRENTLY ONLINE USERS\n");
                printf ("======================\n");
                while (p)
                {
                    if (n == 1)
                    {
                        int clientCount = atoi(p);
                        if (clientCount == 1)
                        {
                            printf ("No other user currently online\n\n");
                            break;
                        }
                        else
                        {
                            printf ("%d other user(s) currently online\n\n", clientCount - 1);
                        }
                    }
                    else
                    {

                        p = strtok(NULL, " "); 
                        if (p != NULL)
                        {
                            if (strcmp(p, this->getUserName()) != 0)
                                printf ("%s\n", p);
                        }
                    }
                    n++;
                }
                
            }
            cout << endl << endl;
            break;

        case NACK:
        case OFFLINE_INFO:
        case ONLINE_INFO:
            {
                string msg(recvHeader->attributes[0].payload.message);
                cout << endl << msg << endl;
            }
            break;
    }
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

int Client::getSocketFd()
{
    return sockFd;
}

char * Client::getUserName()
{
    return userName;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: client <username> <server IP> <server PORT>\n");
        exit(1);
    }
    Client *c = new Client(argv[1], argv[2], argv[3]);

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
        cout << "Connection successful..\n";
        SBMPHeaderT * sbmpHeader = createMessagePacket(JOIN, c->getUserName(), NULL);
        c->sendData(sbmpHeader, sizeof(SBMPHeader));
        c->recvData();
    }

    int clientFd = c->getSocketFd();
    fd_set read_fds;
    int fdMax;
    fdMax = clientFd;
    char message[512];

    cout << "You: ";
    fflush(stdout);

    bool hasPrinted;

    while(1)
    {
        hasPrinted = false;
        FD_ZERO(&read_fds);
        FD_SET(c->getSocketFd(), &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
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
            if (!hasPrinted)
            {
                cout << "You: ";
                hasPrinted = true;
            }
            cin.getline(message, 512);
            SBMPHeaderT * sbmpHeader = createMessagePacket(SEND, c->getUserName(), message);
            c->sendData(sbmpHeader, sizeof(SBMPHeader));
            fflush(STDIN_FILENO);
        }

        if (FD_ISSET(clientFd, &read_fds))
        {
            /*
             * Get broadcast message from server
             */
            c->recvData();
            if (!hasPrinted)
            {
                cout << "You: ";
                hasPrinted = true;
            }
            fflush(stdout);
            fflush(STDIN_FILENO);
        }
    }

    return 0;
}
