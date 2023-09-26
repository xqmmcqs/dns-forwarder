#ifndef _UTILS_H_
#define _UTILS_H_

#include <arpa/inet.h>
#include <queue>
#include <stdint.h>
#include <string>
#include <sys/epoll.h>

struct sockaddr;

namespace DnsForwarder
{
namespace Wrapper
{
void HostToIp4(const std::string &host, uint32_t &ip);
void HostToIp6(const std::string &host, uint8_t ip[16]);
void SockAddr4(const std::string &host, uint16_t port, sockaddr_in &addr);
void SockAddr6(const std::string &host, uint16_t port, sockaddr_in6 &addr);
int Socket(int domain, int type, int protocol);
void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void Close(int sockfd);
ssize_t SendTo(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t RecvFrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int EpollCreate();
int SetNonBlocking(int sockfd);
void EpollAddFd(int epollfd, int sockfd, epoll_event &event);
void EpollRemoveFd(int epollfd, int sockfd);
int EpollWait(int epollfd, epoll_event *events, int maxevents, int timeout);
} // namespace Wrapper
} // namespace DnsForwarder

#endif