#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#define MAX_BUFFER 100                   /* maximum size of message is MAX_BUFFER - 2 characters */

/* helper functions */
void session_cleanup(void* ptr);

/* thread functions */
void* send_message(void *ptr);          /* sends a UDP datagram */
void* receive_message(void *ptr);       /* awaits a UDP datagram */
void* print_message(void *ptr);         /* prints message to the console */
void* write_message(void *ptr);         /* awaits input from keyboard */

/* threads */
pthread_t send_T;                       /* thread to send messages */
pthread_t recv_T;                       /* thread to receive messages */
pthread_t print_T;                      /* thread to print messages */
pthread_t write_T;                      /* thread to write messages */

/* global info */
struct Data {
    int sockfd;                         /* file descriptor */
    struct addrinfo *servinfo;          /* points to results of getaddrinfo */        

    pthread_mutex_t messages_in_lock;   /* mutex lock for incoming messages */
    pthread_mutex_t messages_out_lock;  /* mutex lock for outgoing messages */

    LIST* messages_in;                  /* list of messages that have been received from remote client */
    LIST* messages_out;                 /* list of messages that will be sent to remote client */

    pthread_cond_t messages_to_print;   /* checks if the receive_message thread has added to messages_in list */
    pthread_cond_t messages_to_send;    /* checks if the write_message thread has added to messages_out list */
};

int main(int argc, char* argv[]) {

    /* check command line arguments */
    if (argc < 4) {
        printf("Usage: %s [local port] [local machine name] [local port]\n", argv[0]);
        return 0;
    }
    const char* local_port = argv[1];
    const char* remote_machine_name = argv[2];
    const char* remote_port = argv[3];
    struct Data* shared_data = (struct Data*)malloc(sizeof(struct Data));

    /* construct the local socket address */
    int status = 0;
    struct addrinfo local_hints;                    /* address info */
    memset(&local_hints, 0, sizeof(local_hints));   /* make sure struct is empty */

    local_hints.ai_family = AF_INET;                /* IPv4 */
    local_hints.ai_socktype = SOCK_DGRAM;           /* UDP datagram socket */
    local_hints.ai_protocol = 0;                    /* default protocol */
    local_hints.ai_flags = AI_PASSIVE;              /* bind to the IP of the host this program is running on */
    
    /* address info of local socket */
    if ((status = getaddrinfo(NULL, local_port, &local_hints, &shared_data->servinfo)) != 0) {
        printf("ERROR: failed to resolve local socket address\n");
        return 0;
    }

    /* create socket */
    shared_data->sockfd = socket(shared_data->servinfo->ai_family, shared_data->servinfo->ai_socktype, shared_data->servinfo->ai_protocol);
    if (shared_data->sockfd < 0) { 
        printf("ERROR: failed to open socket\n"); 
        return 0;
    }

    /* bind the local address to the socket */
    if (bind(shared_data->sockfd, shared_data->servinfo->ai_addr, shared_data->servinfo->ai_addrlen) < 0 ) {
        printf("error, bind failed\n");
        return 0;
    }

    /* address info of remote socket */
    if ((status = getaddrinfo(remote_machine_name, remote_port, &local_hints, &shared_data->servinfo)) != 0) {
        printf("ERROR: failed to resolve remote socket address\n");
        return 0;
    }; 

    printf("s-talk established with %s\n", remote_machine_name);

    /* initialize lists */
    shared_data->messages_in = ListCreate();
    shared_data->messages_out = ListCreate();

    /* initialize mutex lock */
    pthread_mutex_init(&shared_data->messages_in_lock, NULL);
    pthread_mutex_init(&shared_data->messages_out_lock, NULL);

    /* initialize condition variables */
    pthread_cond_init(&shared_data->messages_to_print, NULL);
    pthread_cond_init(&shared_data->messages_to_send, NULL);

    /* initialize threads */
    send_T = 0;
    recv_T = 0;
    print_T = 0;
    write_T = 0;

    /* spawn 4 threads to send messages, receive messages, print messages, and write messages */
    pthread_create(&write_T, NULL, write_message, shared_data);
    pthread_create(&send_T, NULL, send_message, shared_data);
    pthread_create(&recv_T, NULL, receive_message, shared_data);
    pthread_create(&print_T, NULL, print_message, shared_data);

    pthread_join(send_T, NULL);
    pthread_join(recv_T, NULL);
    pthread_join(print_T, NULL);
    pthread_join(write_T, NULL);

    return 0;
}

/* thread function that awaits a UDP datagram message.
 * it adds the received message onto the messages_in_list. */
void* receive_message(void* ptr) {

    struct Data* shared_data = (struct Data*)ptr;
    struct sockaddr_storage src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    char msg[MAX_BUFFER];

    while(1) {
        ssize_t len = recvfrom(shared_data->sockfd, 
                                     msg, 
                                     MAX_BUFFER, 
                                     MSG_WAITALL, 
                                     (struct sockaddr*)&src_addr, 
                                     &src_addr_len);
        msg[len] = '\0';                                       /* null termination character */

        /* s-talk termination */
        if (strcmp((char*)msg, "!\n") == 0) { 
            printf("session terminated by remote user\n");

            /* cancel all other threads */
            pthread_cancel(send_T);
            pthread_cancel(write_T);
            pthread_cancel(print_T);

            session_cleanup(ptr);

            /* exit this thread */
            pthread_exit(NULL);
        }

        /* obtain lock and add message to the list */
        pthread_mutex_lock(&shared_data->messages_in_lock);
        ListPrepend(shared_data->messages_in, (char*)msg);     /* critical section */

        /* signal the print_message thread to go and release the lock */
        pthread_cond_signal(&shared_data->messages_to_print);
        pthread_mutex_unlock(&shared_data->messages_in_lock);
    }

    return NULL;
}

/* thread function that prints characters (message) to the console.
 * it takes each message from the messages_in_list and prints it to the console. */
void* print_message(void *ptr) {

    struct Data* shared_data = (struct Data*)ptr;
    char* msg = NULL;

    while(1) {

        /* obtain lock */
        pthread_mutex_lock(&shared_data->messages_in_lock);

        /* pop the message from list when list is not empty */
        while(ListCount(shared_data->messages_in) <= 0) { 
            pthread_cond_wait(&shared_data->messages_to_print, &shared_data->messages_in_lock);
        }
        msg = ListTrim(shared_data->messages_in);           /* critical section */

        /* release lock */
        pthread_mutex_unlock(&shared_data->messages_in_lock);

        /* print message to console */
        printf("%s", msg);
    }

    return NULL;
}


/* thread function that awaits input from keyboard. 
 * on receipt of keyboard input, it adds the input message to the messages_out_list */
void* write_message(void *ptr) {

    struct Data* shared_data = (struct Data*)ptr;
    char msg[MAX_BUFFER];

    while(1) {

        /* fetch message from stdin into msg */
        fgets(msg, MAX_BUFFER, stdin);

        /* s-talk termination */
        if (strcmp(msg, "!\n") == 0) { 

            printf("terminating s-talk session\n");
            sleep(3);       /* let any remaining messages in the list be processed */

            /* cancel the send_message thread */
            pthread_cancel(send_T);

            /* send the terminating message to the receiver */
            if (sendto(shared_data->sockfd, 
                        msg,
                        strlen(msg),
                        0, 
                        shared_data->servinfo->ai_addr, 
                        shared_data->servinfo->ai_addrlen) == -1) {
                printf("ERROR: cannot send terminating message\n");
                return(0);
            }

            session_cleanup(ptr);

            /* cancel the other threads */
            pthread_cancel(recv_T);
            pthread_cancel(print_T);

            /* exit this thread */
            pthread_exit(NULL);
        }

        /* obtain lock and add message to list */
        pthread_mutex_lock(&shared_data->messages_out_lock);
        ListPrepend(shared_data->messages_out, (char*)msg);     /* critical section */

        /* signal the send_message thread to go and release the lock */
        pthread_cond_signal(&shared_data->messages_to_send);
        pthread_mutex_unlock(&shared_data->messages_out_lock);
    }

    return NULL;
}

/* thread function to send a UDP datagram.
 * it takes a message from the messages_out_list and sends it to the remote client. */
void* send_message(void* ptr) {

    struct Data* shared_data = (struct Data*)ptr;
    char* msg = NULL;

    while(1) {

        /* obtain lock */
        pthread_mutex_lock(&shared_data->messages_out_lock);

        /* pop the message from list when list is not empty */
        while (ListCount(shared_data->messages_out) <= 0) {
            pthread_cond_wait(&shared_data->messages_to_send, &shared_data->messages_out_lock);
        }
        msg = ListTrim(shared_data->messages_out);              /* critical section */

        /* release the lock */
        pthread_mutex_unlock(&shared_data->messages_out_lock);

        /* send message to remote address */
        if (sendto(shared_data->sockfd, 
                    msg,
                    strlen(msg),
                    0, 
                    shared_data->servinfo->ai_addr, 
                    shared_data->servinfo->ai_addrlen) == -1) {
            printf("ERROR: in sendto\n");
            return(0);
        }
    }

    return NULL;
}

/* cleanup resources used by s-talk session */
void session_cleanup(void* ptr) {

    struct Data* shared_data = (struct Data*)ptr;

    /* close socket */
    close(shared_data->sockfd);

    /* figure out how to free the list in case there is anything left in them */

    /* free any other memory */
    freeaddrinfo(shared_data->servinfo);

    /* unlock the mutex locks and destroy them */
    pthread_mutex_unlock(&shared_data->messages_out_lock);
    pthread_mutex_unlock(&shared_data->messages_in_lock);

    pthread_mutex_destroy(&shared_data->messages_in_lock);
    pthread_mutex_destroy(&shared_data->messages_out_lock);

    /* decrement wrefs and destroy the condition variables */
    shared_data->messages_to_print.__data.__wrefs = 0;
    shared_data->messages_to_send.__data.__wrefs = 0;

    pthread_cond_destroy(&shared_data->messages_to_print);
    pthread_cond_destroy(&shared_data->messages_to_send);
}
