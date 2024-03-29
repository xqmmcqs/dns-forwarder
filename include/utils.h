#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/time.h>

#include <iostream>
#include <queue>
#include <string>

struct sockaddr;

namespace DnsForwarder
{
namespace Wrapper
{
void HostToIp4(const std::string &host, uint32_t &ip);
void HostToIp6(const std::string &host, uint8_t ip[16]);
void IpToHost4(uint32_t ip, std::string &host);
void IpToHost6(const uint8_t ip[16], std::string &host);
void SockAddr4(const std::string &host, uint16_t port, sockaddr_in &addr);
void SockAddr6(const std::string &host, uint16_t port, sockaddr_in6 &addr);
int Socket(int domain, int type, int protocol);
void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void Listen(int sockfd, int backlog);
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void Close(int sockfd);
void SetSockOpt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
ssize_t SendTo(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t RecvFrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t Send(int sockfd, const void *buf, size_t len, int flags);
ssize_t Recv(int sockfd, void *buf, size_t len, int flags);
ssize_t Read(int fd, void *buf, size_t count);
int EpollCreate();
int SetNonBlocking(int sockfd);
void EpollAddFd(int epollfd, int sockfd, void *ptr, uint32_t events);
void EpollDelFd(int epollfd, int sockfd);
void EpollModFd(int epollfd, int sockfd, void *ptr, uint32_t events);
int EpollWait(int epollfd, epoll_event *events, int maxevents, int timeout);
void SigEmptySet(sigset_t *set);
void SigAddSet(sigset_t *set, int signum);
void SigProcMask(int how, const sigset_t *set, sigset_t *oldset);
int SignalFd(const sigset_t *mask);
void GetITimer(int which, struct itimerval *curr_value);
void SetITimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
} // namespace Wrapper

class Logger
{
  public:
    enum LogLevel
    {
        NONE = 0,
        ERROR = 1,
        WARNING = 2,
        INFO = 3,
        DEBUG = 4
    };
    static Logger GetInstance();
    static void SetLevel(LogLevel level);
    static std::string RawDataFormatter(const std::string &raw);
    static std::string SocketFormatter(const sockaddr_in &addr);
    static std::string SocketFormatter(const sockaddr_in6 &addr);
    void Log(std::string filename, int line, LogLevel message_level, const std::string &message);

  private:
    static LogLevel m_log_level;
};

} // namespace DnsForwarder