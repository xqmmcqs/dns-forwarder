#include "utils.h"
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>

using namespace std;

void DnsForwarder::Wrapper::HostToIp4(const string &host, uint32_t &ip)
{
    if (!inet_pton(AF_INET, host.c_str(), &ip))
        throw invalid_argument("Invalid IPv4 address");
}

void DnsForwarder::Wrapper::HostToIp6(const string &host, uint8_t ip[16])
{
    if (!inet_pton(AF_INET6, host.c_str(), ip))
        throw invalid_argument("Invalid IPv6 address");
}

void DnsForwarder::Wrapper::SockAddr4(const std::string &host, uint16_t port, sockaddr_in &addr)
{
    addr.sin_family = AF_INET;
    HostToIp4(host, addr.sin_addr.s_addr);
    addr.sin_port = htons(port);
}

void DnsForwarder::Wrapper::SockAddr6(const std::string &host, uint16_t port, sockaddr_in6 &addr)
{
    addr.sin6_family = AF_INET6;
    HostToIp6(host, addr.sin6_addr.s6_addr);
    addr.sin6_port = htons(port);
}

int DnsForwarder::Wrapper::Socket(int domain, int type, int protocol)
{
    int fd = socket(domain, type, protocol);
    if (fd == -1)
        throw runtime_error("Failed to create socket");
    return fd;
}

void DnsForwarder::Wrapper::Bind(int sockfd, const sockaddr *addr, socklen_t addrlen)
{
    if (bind(sockfd, addr, addrlen) == -1)
        throw runtime_error("Failed to bind socket");
}

void DnsForwarder::Wrapper::Close(int sockfd)
{
    if (close(sockfd) == -1)
        throw runtime_error("Failed to close socket");
}

ssize_t DnsForwarder::Wrapper::SendTo(int sockfd, const void *buf, size_t len, int flags, const sockaddr *dest_addr,
                                      socklen_t addrlen)
{
    int nsend = sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    if (nsend == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        else
            throw runtime_error("Failed to send data");
    }
    return nsend;
}

ssize_t DnsForwarder::Wrapper::RecvFrom(int sockfd, void *buf, size_t len, int flags, sockaddr *src_addr,
                                        socklen_t *addrlen)
{
    int nrecv = recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (nrecv == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        else
            throw runtime_error("Failed to receive data");
    }
    return nrecv;
}

int DnsForwarder::Wrapper::EpollCreate()
{
    int epollfd = epoll_create1(0);
    if (epollfd == -1)
        throw runtime_error("Failed to create epoll");
    return epollfd;
}

int DnsForwarder::Wrapper::SetNonBlocking(int sockfd)
{
    int old_flags = fcntl(sockfd, F_GETFL, 0);
    if (old_flags == -1)
        throw runtime_error("Failed to get socket flags");
    int new_flags = old_flags | O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, new_flags) == -1)
        throw runtime_error("Failed to set socket flags");
    return old_flags;
}

void DnsForwarder::Wrapper::EpollAddFd(int epollfd, int sockfd, epoll_event &event)
{
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event) == -1)
        throw runtime_error("Failed to add socket to epoll");
    SetNonBlocking(sockfd);
}

void DnsForwarder::Wrapper::EpollRemoveFd(int epollfd, int sockfd)
{
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, nullptr) == -1)
        throw runtime_error("Failed to remove socket from epoll");
}

int DnsForwarder::Wrapper::EpollWait(int epollfd, epoll_event *events, int maxevents, int timeout)
{
    int nready = epoll_wait(epollfd, events, maxevents, timeout);
    if (nready == -1)
        throw runtime_error("Failed to wait for epoll events");
    return nready;
}
