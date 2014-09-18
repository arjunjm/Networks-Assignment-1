#ifndef __HELPER_H__
#define __HELPER_H__

#include <sys/types.h>
#include <sys/wait.h>

#define MAXDATASIZE 100

struct sockaddr;
struct sockaddr_in;

typedef enum AttributeType : unsigned short
{
   ATTR_REASON,
   ATTR_USER,
   ATTR_CLI_CNT,
   ATTR_MSG
} AttributeTypeT;

typedef union
{
    char username[16];
    char message[512];
    char reason[32];
    unsigned short clientCount;
}Payload;

typedef struct SBMPAttribute
{
    AttributeTypeT type;
    unsigned short length;
    Payload payload;
} __attribute__((packed)) SBMPAttributeT;

typedef struct SBMPHeader
{
    unsigned int version : 9;
    unsigned int type : 7;
    unsigned short length;
    SBMPAttributeT attributes[2];
} __attribute__((packed)) SBMPHeaderT;

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
