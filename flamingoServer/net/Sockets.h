#pragma once
#include <stdint.h>
#include "../base/platform.h"

struct tcp_info;

namespace net {
  class InetAddress;
  class Socket {
  public:
    explicit Socket(int sockfd) : sockFd(sockfd) {}
    ~Socket();
    SOCKET fd() const { return sockFd; }
    // 如果成功返回true
    bool getTcpInfoString(char* buf, int len) const;
    void bindAddress(const InetAddress& localAddr);
    void listen();
    int accept(InetAddress* peeraddr);
    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
  private: 
    const SOCKET sockFd;
  };

  namespace sockets {
    SOCKET createOrDie();
    SOCKET createNonblockingOrDie();
    void setNonBlockAndCloseOnExec(SOCKET sockfd);
    void setReuseAddr(SOCKET sockfd, bool on);
    void setReusePort(SOCKET sockfd, bool on);

    SOCKET connect(SOCKET sockfd, const struct sockaddr_in& addr);
    void binOrDie(SOCKET sockfd, const struct sockaddr_in& addr);
    void listenOrDie(SOCKET sockfd);
    SOCKET accept(SOCKET sockfd, struct sockaddr_in& addr);
    int32_t read(SOCKET sockfd, void* buf, int32_t count);
#ifndef
    ssize_t readv(SOCKET sockfd, const struct iovec *iov, int iovcnt);
#endif
    
  }
}