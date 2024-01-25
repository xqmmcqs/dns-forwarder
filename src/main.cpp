#include <getopt.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "server.h"
#include "utils.h"

using namespace std;
using namespace DnsForwarder;

int main(int argc, char *argv[])
{
    Logger::GetInstance().SetLevel(Logger::INFO);
    string local_ip4("0.0.0.0");
    string local_ip6("::1");
    uint16_t local_port(10053);
    string nameservers("114.114.114.114");

    while (true)
    {
        static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                               {"debug", no_argument, 0, 'd'},
                                               {"local-ipv4", required_argument, 0, 0},
                                               {"local-ipv6", required_argument, 0, 0},
                                               {"local-port", required_argument, 0, 0},
                                               {"nameservers", required_argument, 0, 0},
                                               {0, 0, 0, 0}};
        int option_index = 0;
        int c = getopt_long(argc, argv, "hd", long_options, &option_index);
        if (c == -1)
            break;
        switch (c)
        {
        case 0:
            switch (option_index)
            {
            case 2:
                local_ip4 = optarg;
                break;
            case 3:
                local_ip6 = optarg;
                break;
            case 4:
                local_port = stoi(optarg);
                break;
            case 5:
                nameservers = optarg;
                break;
            }
            break;
        case 'h':
            cout << "Usage: dns-forwarder [OPTION]..." << endl;
            cout << "Forward DNS queries to remote DNS server" << endl;
            cout << endl;
            cout << "  -h, --help                    display this help and exit" << endl;
            cout << "  -d, --debug                   enable debug mode" << endl;
            cout << "      --local-ipv4=IP           local IPv4 address (default: 127.0.0.1)" << endl;
            cout << "      --local-ipv6=IP           local IPv6 address (default: ::1)" << endl;
            cout << "      --local-port=PORT         local port (default: 10053)" << endl;
            cout << "      --nameservers=IP1,IP2,... Do53 nameservers (default: 114.114.114.114)" << endl;
            return 0;
        case 'd':
            Logger::GetInstance().SetLevel(Logger::DEBUG);
            break;
        case '?':
        default:
            break;
        }
    }

    sockaddr_in local_addr4;
    sockaddr_in6 local_addr6;
    Wrapper::SockAddr4(local_ip4, local_port, local_addr4);
    Wrapper::SockAddr6(local_ip6, local_port, local_addr6);

    sigset_t signal_mask;
    Wrapper::SigEmptySet(&signal_mask);
    Wrapper::SigAddSet(&signal_mask, SIGINT);
    Wrapper::SigAddSet(&signal_mask, SIGTERM);
    Wrapper::SigAddSet(&signal_mask, SIGALRM);
    Wrapper::SigProcMask(SIG_BLOCK, &signal_mask, NULL);

    int signalfd = Wrapper::SignalFd(&signal_mask);
    Server server(local_addr4, local_addr6, signalfd);

    replace(nameservers.begin(), nameservers.end(), ',', ' ');
    istringstream ss(nameservers);
    string remote_ip;
    while (ss >> remote_ip)
    {
        if (remote_ip.find('.') != string::npos)
        {
            sockaddr_in remote_addr4;
            Wrapper::SockAddr4(remote_ip, 53, remote_addr4);
            server.AddRemote(remote_addr4);
        }
        else
        {
            sockaddr_in6 remote_addr6;
            Wrapper::SockAddr6(remote_ip, 53, remote_addr6);
            server.AddRemote(remote_addr6);
        }
    }

    auto logger = Logger::GetInstance();
    logger.Log(__FILE__, __LINE__, Logger::INFO, "Running dns-forwarder.");
    server.Run();
    return 0;
}