## Terminal-to-Terminal Chat Program using the UNIX User Datagram Protocol (UDP)

### About the program
- The program allows users on different computers (or 2 terminals on the same computer) to communicate through the console.
- The program uses network sockets and IPC to allow different processes to communicate over the internet
- communication is done via UDP datagrams, which is an internet data transfer protocol

### What is User Datagram Protocol (UDP)? 
- UDP is a connectionless, loss-tolerating data transfer protocol, meaning no handshake is required before data can be sent, and no error-checking/resending of packets is done by the receiver upon receiving a message.
- UDP differs from TCP, which is another data transfer protocol that is connection-based and loss-intolerant.

### What is Inter-Process Communication (IPC)?
- IPC is the mechanisms that allows processes to communicate with each other and synchronize their actions
- Typically, IPC is used to talk about communication between different processes, running within the same system


### Starting the program
Compile the program and initiate a chat with a client at a remote terminal using

```
./s-talk [my port #] [remote machine IP] [remote port #]
```

### During the chat
* type a single '!' to terminate s-talk session

- Program uses pthreads to manage 4 concurrent functions:
1. keyboard input for writing messages
2. sending these written messages to remote UNIX process using UDP
3. receiving messages from remote client 
4. printing these received messages to console


