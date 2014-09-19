/*
 * Implementation of the Server Class
 */

#include "server.h"
#include <iostream>

using namespace std;

Server::Server()
{
    memset(&this->hints, 0, sizeof hints);
    this->hints.ai_family = AF_UNSPEC;
    this->hints.ai_socktype = SOCK_STREAM;
    this->hints.ai_flags = AI_PASSIVE; // use this machine's IP
}

int Server::createSocketAndBind()
{
    struct addrinfo *serverInfo, *temp;
    int yes = 1;
    int rv;
    if ((rv = getaddrinfo(NULL, PORT, &this->hints, &serverInfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    /*
     * Loop through the results and bind to the first we can
     */
    for(temp = serverInfo; temp != NULL; temp = temp->ai_next)
    {
        sockFd = socket(temp->ai_family, temp->ai_socktype,
                temp->ai_protocol);
        if (sockFd == -1)
        {
            perror("Unable to create socket. Attempting again..");
            continue;
        }

        if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, 
                    sizeof(int)) == -1)
        {
            perror("setsockopt failed");
            exit(1);
        }

        if (bind(sockFd, temp->ai_addr, temp->ai_addrlen) == -1)
        {
            close(sockFd);
            perror("server: Unable to bind");
            continue;
        }
        break;
    }

    freeaddrinfo(serverInfo);

    if (temp == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    else
    {
        // success!
        return 0;
    }
}

int Server::listenForConnections()
{
    if(listen(sockFd, BACKLOG) == -1)
    {
        perror("Error while listening");
        exit(1);
    }
    return 0;
}

int Server::sendData(int sockFD, void * buf, size_t len, int flags)
{
    int retVal;
    retVal = send(sockFD, buf, len, flags);
    return retVal;
}
/*
void printMap(std::map<int, string> myMap)
{
    std::map<int, string>::iterator it;
    for (it = myMap.begin(); it != myMap.end(); it++)
    {
        cout << it->first << " : " << it->second << endl;   
    }
}
*/
int Server::recvData(int sockFD, SBMPMessageType &msgType, char *message)
{
    int numBytes;
    SBMPHeaderT *recvHeader = new SBMPHeaderT();
    numBytes = recv(sockFD, recvHeader, sizeof(SBMPHeaderT)-1, 0);
    if (numBytes == -1)
    {
        perror("Error in receiving data from the client");
        exit(1);
    }

    msgType = (SBMPMessageTypeT) recvHeader->type;

    switch(msgType) 
    {
        case JOIN:
            {
                string userName(recvHeader->attributes[0].payload.username);
                if (userStatusMap.find(userName) == userStatusMap.end())
                {
                    cout << "======================================" << endl;
                    cout << userName << " has joined the chat session" << endl;
                    cout << "======================================" << endl;
                    userStatusMap[userName] = ONLINE;
                    fdUserMap[sockFD] = userName;
                }
                else
                {
                    cout << "The user has already connected\n";
                }
            }
            break;

        case FWD:
            break;

        case SEND:
            strcpy(message, recvHeader->attributes[0].payload.message);
            cout << " Received message from " << fdUserMap[sockFD] << ": " << message << endl;
            break;

    }
    
    return numBytes;
}

int Server::acceptConnection()
{
    struct sockaddr_storage clientAddr;
    socklen_t sin_size;
    char ipAddr[INET6_ADDRSTRLEN]; 
    fd_set master, read_fds;
    int fdMax;
    struct timeval tv;
    tv.tv_sec = 50;
    tv.tv_usec = 500000;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(sockFd, &master);
    fdMax = sockFd;
    while(1)
    {
       read_fds = master;
       if (select(fdMax + 1, &read_fds, NULL, NULL, &tv) == -1)
       {
           perror("Select");
           exit(4);
       }

       /*
        * Run through existing connections and check if a client is ready
        */
       for (int i = 0; i <= fdMax; i++)
       {
           if (FD_ISSET(i, &read_fds))
           {
               if (i == sockFd)
               /*
                * This is the listening socket. 
                * Check new incoming connections.
                */
               {
                   sin_size = sizeof clientAddr;
                   newConnFd = accept(i, (struct sockaddr *)&clientAddr, &sin_size);
                   if (newConnFd == -1)
                   {
                       perror("Error while accepting connection..");
                   }
                   else
                   {
                       FD_SET(newConnFd, &master);

                       if (newConnFd > fdMax)
                       {
                           fdMax = newConnFd;
                       }
                       inet_ntop(clientAddr.ss_family, get_in_addr((struct sockaddr *)&clientAddr), ipAddr, sizeof ipAddr);
                       printf("server: got connection from %s\n", ipAddr);

                   }

               }
               else
               {
                   /*
                    * Handle data from client connection.
                    */
                   int readBytes;
                   SBMPMessageType msgType;
                   char *message = new char[512];
                   if ((readBytes = recvData(i, msgType, message)) <= 0)
                   {
                        /*
                         * This means either there was a error in receiving data
                         * or that the client has closed the connection.
                         */
                        if (readBytes == 0)
                        {
                            // Close the connection
                            printf("server: closing connection");
                            FD_CLR(i, &master);
                            close(i);
                        }
                        else
                        {
                            perror("receive");
                        }
                   }
                   else
                   {
                       /*
                        * Client is sending actual data.
                        */
                       for (int j = 0; j <= fdMax; j++)
                       {
                           if (FD_ISSET (j, &master))
                           {
                               if ((j != i) && (j != sockFd))
                               {
                                   if (msgType == SEND)
                                   {
                                       cout << " Sending message: " << message << endl;
                                       if (sendData(j, message, strlen(message), 0) == -1)
                                           perror("Error while broadcasting message");
                                   }
                               }
                           }
                       }
                       delete [] message;
                        
                   }
               }
           }
       }
    }
    return 0;
}

int main()
{
    Server *s = new Server();
    s->createSocketAndBind();
    s->listenForConnections();

    printf("server: waiting for connections..\n");
    s->acceptConnection();
    return 0;
}
