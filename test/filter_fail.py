import dns.resolver

nameservers = [
    "166.111.8.28",
]

with open("test/top500.txt", "r") as f:
    with open("test/fail.txt", "w") as f2:
        for line in f:
            res = dns.resolver.make_resolver_at(nameservers[0], port=53)
            test_case = line.strip()
            try:
                answer = res.resolve(qname=test_case, rdtype="A")
            except dns.resolver.NXDOMAIN:
                # pass
                f2.write(f"{test_case}\n")
            except dns.resolver.NoAnswer:
                # pass
                f2.write(f"{test_case}\n")
            except dns.resolver.NoNameservers:
                # pass
                f2.write(f"{test_case}\n")
            except dns.resolver.LifetimeTimeout:
                print(f"timeout {test_case}")
                raise
