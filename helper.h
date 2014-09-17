#ifndef __HELPER_H__
#define __HELPER_H__

#include <sys/types.h>
#include <sys/wait.h>

struct sockaddr;
struct sockaddr_in;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

#endif /* __HELPER_H__ */
