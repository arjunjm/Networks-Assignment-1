#include "server.h"
#include "helper.h"

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

int Server::acceptConnection()
{
    struct sockaddr_storage clientAddr;
    socklen_t sin_size;
    char ipAddr[INET6_ADDRSTRLEN]; 
    while(1)
    {
       sin_size = sizeof clientAddr;
       newConnFd = accept(sockFd, (struct sockaddr *)&clientAddr, &sin_size);

       if (newConnFd == -1)
       {
           perror("Error while accepting connection..");
           continue;
       }

       inet_ntop(clientAddr.ss_family, get_in_addr((struct sockaddr *)&clientAddr), ipAddr, sizeof ipAddr);
       printf("server: got connection from %s\n", ipAddr);

       if (!fork()) { // this is the child process
           close(sockFd); // child doesn't need the listener
           if (send(newConnFd, "Hello, world!", 13, 0) == -1)
               perror("Error while sending message");
           close(newConnFd);
           exit(0);
       }
       close(newConnFd);  // parent doesn't need this

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
