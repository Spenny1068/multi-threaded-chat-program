## Terminal-to-Terminal Chat Program using the UNIX User Datagram Protocol (UDP)

### About the program
- The program allows users on different terminals to send UDP Datagrams back and forth to communicate with each other
- UDP is a connectionless, loss-tolerating data transfer protocol, meaning no handshake is required before data can be sent, and no error-checking/resending of packets is done by the receiver upon receiving a message.
- IPC (Inter-process communication) is used between threads within the same process that handle sending/receiving messages

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


