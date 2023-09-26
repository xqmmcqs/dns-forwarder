import dns.resolver
import subprocess
import atexit


p = subprocess.Popen(["./build/dns-forwarder"])
atexit.register(p.terminate)

test_res = dns.resolver.make_resolver_at("127.0.0.1", port=10053)
res = dns.resolver.make_resolver_at("192.168.3.1", port=53)

test_case_list = [
    {"qname": "baidu.com", "rdtype": "A"},
    {"qname": "byr.pt", "rdtype": "AAAA"},
    {"qname": "xqmmcqs.com", "rdtype": "CNAME"},
    {"qname": "xqmmcqs.com", "rdtype": "MX"},
    {"qname": "xqmmcqs.com", "rdtype": "TXT"},
]

for test_case in test_case_list:
    test_answer = test_res.resolve(**test_case)
    answer = res.resolve(**test_case)
    assert test_answer.rrset == answer.rrset
