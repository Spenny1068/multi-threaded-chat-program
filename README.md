GITHUB: 
Multi-threaded chat program using the UNIX User Datagram Protocol (UDP) Inter Process Communication (IPC)

- Compile the program and initiate a chat with a client at a remote terminal using  
  ./s-talk [my port #] [remote machine IP] [remote port #]

- type a single '!' to terminate s-talk session

- Program uses pthreads to manage 4 concurrent functions:
1. keyboard input for writing messages
2. sending these written messages to remote UNIX process using UDP
3. receiving messages from remote client 
4. printing these received messages to console
