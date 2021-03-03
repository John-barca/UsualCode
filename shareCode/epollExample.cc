#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>

#define MAXLINE 1024
#define OPEN_MAX 16
#define SERV_PORT 10001

int main () {
    int i, maxi;
    int listenfd, connfd, sockfd, epfd, nfds;
    int n;
    char buf[MAXLINE];
    struct epoll_event ev, events[20];
    socklen_t clilen;
    struct pollfd client[OPEN_MAX];

    struct sockaddr_in cliaddr, servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    

    return 0;
}