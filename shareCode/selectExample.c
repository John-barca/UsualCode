#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

#define MAXLINE 1024
#define SERV_PORT 10001

int main () {
    int i, maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[FD_SETSIZE]; // nready 描述字的数量
    int n;
    // 创建描述符集合，因为需要比对所以创建两个集合
    fd_set rset, allset;
    char buf[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    // 创建 socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // 定义服务sockaddr_in
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // 绑定socket 
    bind(listenfd, (struct sockaddr*)& servaddr, sizeof(servaddr));
    listen(listenfd, 100);
    // maxfd，用于 select 的第一个参数
    maxfd = listenfd;
    // client 的数量，用于轮询
    maxi = -1;
    // 初始化为-1
    for (i = 0; i < FD_SETSIZE; ++i) 
        client[i] = -1;
    // 清空空间描述符集
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset); // 添加至文件描述符集
    for (;;) {
        rset = allset;
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if (FD_ISSET(listenfd, &rset)) {
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr*)& cliaddr, &clilen);
            for (i = 0; i < FD_SETSIZE; ++i) {
                if (client[i] < 0) {
                    client[i] = connfd;
                    break;
                }
                // 已经满了
                if (i == FD_SETSIZE) {
                    printf("To many clients!\n");
                    return -1;
                }
                FD_SET(connfd, &allset);
                if (connfd > maxfd) maxfd = connfd;
                if (i > maxi) maxi = i;
                if (nready <= 1) continue;
                else nready--;
            }   
            for (i = 0; i <= maxi; ++i) {
                if (client[i] < 0) continue;
                sockfd = client[i];
                if (FD_ISSET(sockfd, &rset)) {
                    n = read(sockfd, buf, MAXLINE);
                    if (n == 0) {
                        // 对端已经关闭，关闭socket文件描述符，并将集合中fd删除
                        close(sockfd);
                        FD_CLR(sockfd, &allset);
                        client[i] = -1;
                    } else {
                        buf[n] = '\0';
                        printf("Socket %d say: %s\n", sockfd.buf);
                        write(sockfd, buf, n);
                    }
                    nready--;
                    if (nready <= 0) break;
                }
            }
        }
    }
    return 0;
}