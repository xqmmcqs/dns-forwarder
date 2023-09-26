#include "dnspacket.h"

using namespace std;

template <> void DnsForwarder::Parse<uint16_t>(std::istringstream &is, uint16_t &t)
{
    char tmp[2];
    is.read(tmp, 2);
    t = ntohs(*reinterpret_cast<uint16_t *>(tmp));
}

template <> void DnsForwarder::Parse<uint32_t>(std::istringstream &is, uint32_t &t)
{
    char tmp[4];
    is.read(tmp, 4);
    t = ntohl(*reinterpret_cast<uint32_t *>(tmp));
}

template <> void DnsForwarder::Serialize<uint16_t>(std::ostringstream &os, const uint16_t &t)
{
    uint16_t tmp = htons(t);
    os.write(reinterpret_cast<char *>(&tmp), 2);
}

template <> void DnsForwarder::Serialize<uint32_t>(std::ostringstream &os, const uint32_t &t)
{
    uint32_t tmp = htonl(t);
    os.write(reinterpret_cast<char *>(&tmp), 4);
}

void DnsForwarder::DomainName::parse(istringstream &is)
{
    char tmp[65];
    while (true)
    {
        if ((is.peek() >> 6) & 0x3)
        {
            is.read(tmp, 2);
            name.append(tmp, 2);
            length += 2;
            return;
        }
        else if (!is.peek())
        {
            is.get();
            length++;
            return;
        }
        int len = is.get();
        is.read(tmp, len);
        tmp[len] = '.';
        name.append(tmp, len + 1);
        length += len + 1;
    }
}

void DnsForwarder::DomainName::serialize(ostringstream &os) const
{
    int pos = 0;
    auto tmp = name.c_str();
    while (pos < name.size())
    {
        int len = name.find('.', pos) - pos;
        if (len < 0)
        {
            os.write(tmp + pos, 2);
            return;
        }
        os.put(len);
        os.write(tmp + pos, len);
        pos += len + 1;
    }
    os.put(0);
}

void DnsForwarder::DnsHeader::parse(std::istringstream &is)
{
    Parse(is, id);
    uint16_t flag;
    Parse(is, flag);
    qr = (flag >> 15) & 0x1;
    opcode = (flag >> 11) & 0xf;
    aa = (flag >> 10) & 0x1;
    tc = (flag >> 9) & 0x1;
    rd = (flag >> 8) & 0x1;
    ra = (flag >> 7) & 0x1;
    z = (flag >> 4) & 0x7;
    rcode = flag & 0xf;
    Parse(is, qdcount);
    Parse(is, ancount);
    Parse(is, nscount);
    Parse(is, arcount);
}

void DnsForwarder::DnsHeader::serialize(std::ostringstream &os) const
{
    Serialize(os, id);
    uint16_t flag = 0;
    flag |= qr << 15;
    flag |= opcode << 11;
    flag |= aa << 10;
    flag |= tc << 9;
    flag |= rd << 8;
    flag |= ra << 7;
    flag |= z << 4;
    flag |= rcode;
    Serialize(os, flag);
    Serialize(os, qdcount);
    Serialize(os, ancount);
    Serialize(os, nscount);
    Serialize(os, arcount);
}

void DnsForwarder::DnsQuestion::parse(std::istringstream &is)
{
    Parse(is, qname);
    Parse(is, qtype);
    Parse(is, qclass);
}

void DnsForwarder::DnsQuestion::serialize(std::ostringstream &os) const
{
    Serialize(os, qname);
    Serialize(os, qtype);
    Serialize(os, qclass);
}

void DnsForwarder::DnsResourceRecord::parse(std::istringstream &is)
{
    Parse(is, name);
    Parse(is, type);
    Parse(is, rclass);
    Parse(is, ttl);
    Parse(is, rdlength);
    char tmp[rdlength];
    is.read(tmp, rdlength);
    rdata = std::move(string(tmp, rdlength));
}

void DnsForwarder::DnsResourceRecord::serialize(std::ostringstream &os) const
{
    Serialize(os, name);
    Serialize(os, type);
    Serialize(os, rclass);
    Serialize(os, ttl);
    Serialize(os, rdlength);
    os.write(rdata.c_str(), rdlength);
}

void DnsForwarder::DnsPacket::parse(std::istringstream &is)
{
    Parse(is, header);
    for (int i = 0; i < header.qdcount; i++)
    {
        DnsQuestion question;
        Parse(is, question);
        questions.push_back(question);
    }
    for (int i = 0; i < header.ancount + header.nscount + header.arcount; i++)
    {
        DnsResourceRecord record;
        Parse(is, record);
        rrs.push_back(record);
    }
}

void DnsForwarder::DnsPacket::serialize(std::ostringstream &os) const
{
    Serialize(os, header);
    for (auto &question : questions)
        Serialize(os, question);
    for (auto &record : rrs)
        Serialize(os, record);
}
