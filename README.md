A project for the Computer Systems course at the University of Melbourne.

Consists of a DNS Server that accepts IPv6 Address requests and servers them by querying servers higher up the hierarchy.
Parses the recieved queries as well as the responses and writes a log of its activities to a dns_svr.log file.
Responds to non-IPv6 (non-AAAA records) requests with Rcode 4 (not implemented).

To run* enter make -B && ./dns_svr <upstream-server-ip> <upstream-server-port> and then enter IPv6 addresses to be queried.

*Note: The project uses the <arpa/inet.h> library and thus needs to be run in a Unix environment.  
