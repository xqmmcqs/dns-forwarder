#include "server.h"
#include "utils.h"
#include <string>
using namespace std;
using namespace DnsForwarder;

int main()
{
    string local_ip("0.0.0.0");
    uint16_t local_port(10053);
    sockaddr_in local_addr4;
    Wrapper::SockAddr4(local_ip, local_port, local_addr4);
    string remote_ip("192.168.3.1");
    uint16_t remote_port(53);
    sockaddr_in remote_addr4;
    Wrapper::SockAddr4(remote_ip, remote_port, remote_addr4);
    sockaddr_in6 local_addr6;
    sockaddr_in6 remote_addr6;
    Server server(local_addr4, local_addr6, remote_addr4, remote_addr6);
    server.Run();
    return 0;
}