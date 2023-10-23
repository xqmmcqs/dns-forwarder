#include "utils.h"

#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

using namespace std;

void DnsForwarder::Wrapper::HostToIp4(const string &host, uint32_t &ip)
{
    if (!inet_pton(AF_INET, host.c_str(), &ip))
        throw invalid_argument("Invalid IPv4 address: " + host);
}

void DnsForwarder::Wrapper::HostToIp6(const string &host, uint8_t ip[16])
{
    if (!inet_pton(AF_INET6, host.c_str(), ip))
        throw invalid_argument("Invalid IPv6 address: " + host);
}

void DnsForwarder::Wrapper::IpToHost4(uint32_t ip, string &host)
{
    char buf[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &ip, buf, INET_ADDRSTRLEN))
        throw invalid_argument("Invalid IPv4 address");
    host = buf;
}

void DnsForwarder::Wrapper::IpToHost6(const uint8_t ip[16], string &host)
{
    char buf[INET6_ADDRSTRLEN];
    if (!inet_ntop(AF_INET6, ip, buf, INET6_ADDRSTRLEN))
        throw invalid_argument("Invalid IPv6 address");
    host = buf;
}

void DnsForwarder::Wrapper::SockAddr4(const string &host, uint16_t port, sockaddr_in &addr)
{
    addr.sin_family = AF_INET;
    HostToIp4(host, addr.sin_addr.s_addr);
    addr.sin_port = htons(port);
}

void DnsForwarder::Wrapper::SockAddr6(const string &host, uint16_t port, sockaddr_in6 &addr)
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

void DnsForwarder::Wrapper::Listen(int sockfd, int backlog)
{
    if (listen(sockfd, backlog) == -1)
        throw runtime_error("Failed to listen on socket");
}

int DnsForwarder::Wrapper::Accept(int sockfd, sockaddr *addr, socklen_t *addrlen)
{
    int fd = accept(sockfd, addr, addrlen);
    if (fd == -1)
        throw runtime_error("Failed to accept connection");
    return fd;
}

void DnsForwarder::Wrapper::Connect(int sockfd, const sockaddr *addr, socklen_t addrlen)
{
    if (connect(sockfd, addr, addrlen) == -1)
        throw runtime_error("Failed to connect to server");
}

void DnsForwarder::Wrapper::Close(int sockfd)
{
    if (close(sockfd) == -1)
        throw runtime_error("Failed to close socket");
}

void DnsForwarder::Wrapper::SetSockOpt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    if (setsockopt(sockfd, level, optname, optval, optlen) == -1)
        throw runtime_error("Failed to set socket option");
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

ssize_t DnsForwarder::Wrapper::Send(int sockfd, const void *buf, size_t len, int flags)
{
    int nsend = send(sockfd, buf, len, flags);
    if (nsend == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        else
            throw runtime_error("Failed to send data");
    }
    return nsend;
}

ssize_t DnsForwarder::Wrapper::Recv(int sockfd, void *buf, size_t len, int flags)
{
    int nrecv = recv(sockfd, buf, len, flags);
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

void DnsForwarder::Wrapper::EpollDelFd(int epollfd, int sockfd)
{
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, nullptr) == -1)
        throw runtime_error("Failed to remove socket from epoll");
}

void DnsForwarder::Wrapper::EpollModifyFd(int epollfd, int sockfd, epoll_event &event)
{
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event) == -1)
        throw runtime_error("Failed to modify socket in epoll");
}

int DnsForwarder::Wrapper::EpollWait(int epollfd, epoll_event *events, int maxevents, int timeout)
{
    int nready = epoll_wait(epollfd, events, maxevents, timeout);
    if (nready == -1)
        throw runtime_error("Failed to wait for epoll events");
    return nready;
}

DnsForwarder::Logger::LogLevel DnsForwarder::Logger::m_log_level = DnsForwarder::Logger::NONE;

DnsForwarder::Logger DnsForwarder::Logger::GetInstance()
{
    static Logger logger_instance;
    return logger_instance;
}

void DnsForwarder::Logger::SetLevel(LogLevel level)
{
    m_log_level = level;
}

void DnsForwarder::Logger::Log(string filename, int line, LogLevel message_level, const string &message)
{
    if (message_level <= m_log_level)
    {
        string log_type;
        switch (message_level)
        {
        case DEBUG:
            log_type = "[DEBUG] ";
            break;
        case INFO:
            log_type = "[INFO] ";
            break;
        case WARNING:
            log_type = "[WARN] ";
            break;
        case ERROR:
            log_type = "[ERROR] ";
            break;
        default:
            log_type = "[NONE] ";
            break;
        }
        cerr << log_type + filename + ":" + to_string(line) + " " + message << endl;
    }
}

string DnsForwarder::Logger::RawDataFormatter(const string &raw)
{
    int counter = 0;
    std::ostringstream ss;
    ss << hex;
    for (int i = 0; i < raw.size(); ++i)
    {
        ss << setw(2) << setfill('0') << static_cast<int>(static_cast<uint8_t>(raw[i])) << ' ';
        ++counter;
        if (counter == 16)
        {
            ss << '\n';
            counter = 0;
        }
    }
    return std::move(ss.str());
}

string DnsForwarder::Logger::SocketFormatter(const sockaddr_in &addr)
{
    string ip;
    Wrapper::IpToHost4(addr.sin_addr.s_addr, ip);
    return ip + ":" + to_string(ntohs(addr.sin_port));
}

string DnsForwarder::Logger::SocketFormatter(const sockaddr_in6 &addr)
{
    string ip;
    Wrapper::IpToHost6(addr.sin6_addr.s6_addr, ip);
    return "[" + ip + "]:" + to_string(ntohs(addr.sin6_port));
}