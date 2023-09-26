#ifndef _DNSPACKET_H_
#define _DNSPACKET_H_

#include <arpa/inet.h>
#include <sstream>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace DnsForwarder
{
enum class DNS_QR
{
    QUERY = 0,
    ANSWER = 1
};

enum class DNS_OPCODE
{
    QUERY = 0,
    IQUERY = 1,
    STATUS = 2
};

enum class DNS_TYPE
{
    A = 1,
    NS = 2,
    CNAME = 5,
    SOA = 6,
    PTR = 12,
    HINFO = 13,
    MINFO = 15,
    MX = 15,
    TXT = 16,
    AAAA = 28
};

enum class DNS_CLASS
{
    IN = 1
};

enum class DNS_RCODE
{
    OK = 0,
    NXDOMAIN = 3
};

struct DomainName
{
    std::string name;
    uint8_t length = 0;
    void parse(std::istringstream &is);
    void serialize(std::ostringstream &os) const;
};

struct DnsHeader
{
    uint16_t id;
    uint8_t qr : 1;
    uint8_t opcode : 4;
    uint8_t aa : 1;
    uint8_t tc : 1;
    uint8_t rd : 1;
    uint8_t ra : 1;
    uint8_t z : 3;
    uint8_t rcode : 4;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
    void parse(std::istringstream &is);
    void serialize(std::ostringstream &os) const;
};

struct DnsQuestion
{
    DomainName qname;
    uint16_t qtype;
    uint16_t qclass;
    void parse(std::istringstream &is);
    void serialize(std::ostringstream &os) const;
};

struct DnsResourceRecord
{
    DomainName name;
    uint16_t type;
    uint16_t rclass;
    uint32_t ttl;
    uint16_t rdlength;
    std::string rdata;
    void parse(std::istringstream &is);
    void serialize(std::ostringstream &os) const;
};

struct DnsPacket
{
    DnsHeader header;
    std::vector<DnsQuestion> questions;
    std::vector<DnsResourceRecord> rrs;
    void parse(std::istringstream &is);
    void serialize(std::ostringstream &os) const;
};

template <typename T> void Parse(std::istringstream &is, T &t)
{
    t.parse(is);
}

template <typename T> void Serialize(std::ostringstream &os, const T &t)
{
    t.serialize(os);
}
} // namespace DnsForwarder

#endif