import dns.resolver
import subprocess
import sys
import atexit


nameservers = [
    "2402:f000:1:801::8:28",
]
test_case_list = [
    {"qname": "baidu.com", "rdtype": "A"},
]

with open("nameserver.txt", "w") as f:
    f.write("\n".join(nameservers))

p = subprocess.Popen(
    ["./build/dns-forwarder", "-d"], stderr=sys.stderr, stdout=sys.stdout
)
atexit.register(p.terminate)

test_res = dns.resolver.make_resolver_at("::1", port=10053)
res = dns.resolver.make_resolver_at(nameservers[0], port=53)

for test_case in test_case_list:
    test_answer = test_res.resolve(**test_case)
    answer = res.resolve(**test_case)
    assert test_answer.rrset == answer.rrset
