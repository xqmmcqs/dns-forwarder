import dns.resolver
import subprocess
import sys
import time
import atexit


nameservers = [
    "166.111.8.28",
]
test_case_list = [
    {"qname": "baidu.com", "rdtype": "A"},
]

p = subprocess.Popen(
    ["./build/dns-forwarder", "-d", "--nameservers=" + ",".join(nameservers)],
    stderr=sys.stderr,
    stdout=sys.stdout,
)
atexit.register(p.terminate)

time.sleep(1)

test_res = dns.resolver.make_resolver_at("127.0.0.1", port=10053)
res = dns.resolver.make_resolver_at(nameservers[0], port=53)

for test_case in test_case_list:
    test_answer = test_res.resolve(**test_case, tcp=True)
    answer = res.resolve(**test_case, tcp=True)
    assert test_answer.rrset == answer.rrset
