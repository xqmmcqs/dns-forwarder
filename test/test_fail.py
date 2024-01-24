import dns.asyncresolver
from dns.exception import DNSException
import subprocess
import asyncio
import sys
import time
import atexit


async def async_dns_query(test_case, server, port):
    try:
        resolver = dns.asyncresolver.Resolver(configure=False)
        resolver.nameservers = [server]
        resolver.port = port
        answer = await resolver.resolve(test_case)
        return (test_case, server, answer, None)
    except DNSException as e:
        return (test_case, server, None, type(e))


async def main():
    nameservers = [
        "166.111.8.28",
    ]

    p = subprocess.Popen(
        ["./build/dns-forwarder", "-d", "--nameservers=" + ",".join(nameservers)],
        stderr=sys.stderr,
        stdout=sys.stdout,
    )
    atexit.register(p.terminate)

    time.sleep(1)

    task_list = []
    with open("test/fail.txt", "r") as f:
        for test_case in f.readlines():
            for server, port in zip([nameservers[0], "127.0.0.1"], [53, 10053]):
                task = asyncio.create_task(
                    async_dns_query(test_case.strip(), server, port)
                )
                task_list.append(task)

    results = await asyncio.gather(*task_list)

    results.sort(key=lambda x: (x[0], x[1]))

    for answer1, answer2 in zip(results[::2], results[1::2]):
        test_case1, server1, ans1, error1 = answer1
        test_case2, server2, ans2, error2 = answer2
        print(answer1, answer2)
        if ans1 is not None:
            pass
            # assert ans1.rrset == ans2.rrset
        else:
            assert error1 == error2


if __name__ == "__main__":
    asyncio.run(main())
