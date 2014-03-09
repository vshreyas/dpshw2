Distributed Password Cracker
============================
Implemented by Shreyas Vinayakumar, Jeyenth Veerarahavan for CSCE 662 Distributed Systems by Radu Stoleru
A basic distributed system that can harness the power of many processors to speed up a compute-intensive task
in the face of packet loss and an unreliable network

Learning goals: 
-features required to create a robust system, 
-handling lost or duplicated Internet packets, as well as failing
clients and servers. 
-creating a set of layered
abstractions in bridging between low-level network protocols and high-level
applications.

This program can be used to find the reverse of the MD5 hash for moderately sized input strings.
It works by dividing the task and assigning it to available agents called as "workers". The workers are co-ordinated by a single server process.
The "server" accepts requests constantly from "client" entities and schedules them optimally among the available workers
All communication between entities is over UDP using the specified protocol.

Prerequisites
==========================
 g++, make, Google Protobufs
 
 To run the code on multiple systems, a LAN network is required

How to build
============================

Run the following command the first time you build:
     make
To remove old executables and build files
     make clean

How to run
============================
 The server is started using the following command, specifying the port
number for the server to use:
./server port
 One or more workers are started using the following command, specifying
the address and port number of the server:
./worker host:port
 When a worker starts, it sends a join request message to the server,
letting the server know that it is available.
 The user generates a cracking request by giving the following command,
specifying the address and port of the server, the hash signature
to be inverted, and the length of the password
./request host:port hash len
 The request client should generate a crack request message giving lower
and upper values aa...a and zz...z, where the number of a’s and z’s is
based on the length of the desired password.
 
 Output
 ============================
 If it finds password pass, it should print
Found: pass
 If it does not find a password, it should print
Not Found
 If the client loses the connection to the server, it should print
Disconnected

Additional features
=============================
Packet loss may be simulated by setting the PACKET_DROP_RATE macro in the file lsp.h
You can observe the effects of dropping packets on the performance