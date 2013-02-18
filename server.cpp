
/*
 * lsp_imp_ser.cpp - lsp server implementation
 * usage: lspserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "que.h"
#define LEN 1024

using namespace std;
typedef struct
{
    uint32_t connid;
    uint32_t seqnum;
    uint8_t payload[50];
} lsp_packet;

typedef struct
{
    que inbox;
    que outbox;
    uint32_t id;
    int sock;
    struct sockaddr_in clientaddr;
    int sent_data;
    int sent_ack;
    int rcvd_data;
    int rcvd_ack;
    pthread_t tid1, tid2, tid3;
    int timeouts;
} args;

pthread_mutex_t mutex1;

void* send_thread(void* a)
{/*
    args* info = (args* )a;
    int fd = info->sock;
    struct sockaddr_in clientaddr = info->clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int n;
    char buf[LEN];
    lsp_packet pkt;
    pkt.connid = info->id;
    pkt.seqnum = 1;
    int last_sent = 0;
    while(1) {
        if(info->seq_send != last_sent) {
            //todo protobuf
            pkt.seqnum = info->seq_send;
            sprintf((char*)pkt.payload, "JOIN%d", pkt.seqnum);
            printf("Sending JOIN%d\n", pkt.seqnum);
            //msg defn in common.h
            //Send a data for testing
            memset(buf, 0, LEN);
            memcpy(buf, &pkt, sizeof(pkt));
            n = sendto(fd, buf, sizeof(pkt), 0, (const sockaddr*)&clientaddr, clientlen);
            if(n < 0)
            {
                perror("ERROR Sending data send_thread");
            }
            else last_sent++;
        }
        //printf("Server send thread Last acked: %d, info->seq_recv: %d\n",info->last_acked,info->seq_recv );

        if (info->last_acked == info->seq_recv - 1)
        {
            pkt.seqnum = info->seq_recv - 1;
            sprintf((char *) pkt.payload, "");
            memset(buf, 0, LEN);
            memcpy(buf, &pkt, sizeof (pkt));
            n = sendto(fd, buf, sizeof (pkt), 0, (const sockaddr*) &clientaddr, clientlen);
            if (n < 0)
            {
                perror("Error in sending ack");
            }
            else info->last_acked++;
        }

    } */
    return NULL;
}

void* recv_thread(void* a)
{
    int n;
    args* info = (args*) a;
    int fd = info->sock;
    struct sockaddr_in clientaddr = info->clientaddr;
    socklen_t clientlen = sizeof (clientaddr);
    char buf[LEN];
    lsp_packet pkt;
    memset(buf, 0, LEN);
    while (1)
    {
        n = recvfrom(fd, buf, sizeof (pkt), 0, (sockaddr*) & clientaddr, &clientlen);
        memcpy(&pkt, buf, sizeof(pkt));
        printf("Server rcv thread got Packet %d %d '%s'",pkt.connid, pkt.seqnum, pkt.payload);
        if (n < 0)
        {
            perror("ERROR Receiving in receiving thread\n");
        }
        if(pkt.payload[0] == '\0')
        {
            if(pkt.seqnum == info->sent_data)
            {
                printf("ACK recieved for message#%d\n", pkt.seqnum);
                info->outbox.deque((char*)pkt.payload) ;
                if(info->rcvd_ack == info->sent_data - 1)info->rcvd_ack++;
            }
            else printf("Duplicate ACK\n");
        }
        else
        {
            if(pkt.seqnum == info->sent_ack + 1)
            {
                printf(" -> server rcv data packet #%d\n", pkt.seqnum);
                //pthread_mutex_
                if(info->inbox.enque((char*)pkt.payload))
                    info->rcvd_data = info->sent_ack + 1;
                else fprintf(stderr, "Queue full");
                //pthread_mutex_
            }
            else if(pkt.seqnum == info->sent_ack)
            {
                printf(" -> server rcvd duplicate data packet#%d, must resend ACK\n", pkt.seqnum);
            }
            else printf(" -> server rcvd out of order Server malfunctioning, sending \n");
        }
    }
    return NULL;
}

void* epoch_thread(void* a)
{
    char buf[LEN];
    args* info = (args*)a;
    int fd = info->sock;
    struct sockaddr_in clientaddr = info->clientaddr;
    socklen_t clientlen = sizeof(info->clientaddr);
    int n;
    lsp_packet pkt;
    pkt.connid = info->id;
    info->timeouts = 0;
    bool idle;
    int prev_ack = 0;
    int prev_data = 0;
    while(1)
    {
        sleep(2);
        idle = true;
        if(info->sent_ack == info->rcvd_data - 1 || info->sent_ack == info->rcvd_data)
        {
            if(info->timeouts == 5){
                printf("No communications from client, disconnecting\n");
                close(info->sock);
                //free(info);
                exit(0);
            }
            pkt.seqnum = info->rcvd_data;
            sprintf((char *) pkt.payload, "");
            memset(buf, 0, LEN);
            memcpy(buf, &pkt, sizeof (pkt));
            printf("Timeout! from epoch handler Sending ack%d\n", pkt.seqnum);
            n = sendto(fd, buf, sizeof (pkt), 0, (const sockaddr*) &clientaddr, clientlen);
            if (n < 0)
            {
                perror("Error in sending ack");
            }
            else
            {
                info->sent_ack = info->rcvd_data;
                if(info->sent_ack != prev_ack)idle = false;
                prev_ack = info->sent_ack;
            }
        }

        if(info->sent_data == info->rcvd_ack || info->sent_data == info->rcvd_ack + 1)
        {
            //todo protobuf
            pkt.seqnum = info->rcvd_ack + 1;
            //
            int rv = info->outbox.que_empty();
            //sprintf((char*)pkt.payload, "x%d", pkt.seqnum);
            if(rv !=0) {
                info->outbox.peek((char*)pkt.payload);
                printf("Timeout! from epoch handler Sending %s %d\n", pkt.payload, pkt.seqnum);
                //msg defn in common.h
                //Send a data for testing
                memset(buf, 0, LEN);
                memcpy(buf, &pkt, sizeof(pkt));
                n = sendto(fd, buf, sizeof(pkt), 0, (const sockaddr*)&clientaddr, clientlen);
                if(n < 0)
                {
                    perror("ERROR Sending data send_thread");
                }
                else
                {
                    info->sent_data = info->rcvd_ack + 1;
                    if(info->sent_data != prev_data)idle = false;
                    prev_data = info->sent_data;
                }
            }
        }
        if(idle == true)info->timeouts++;
        else info->timeouts = 0;

    }
    return NULL;
}

int main()
{
    int sockfd; /* socket */
    int portno; /* port to listen on */
    socklen_t clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */
    char buf[LEN]; /* message buf */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int n; /* message byte size */
    portno = 2700;
    lsp_packet pkt;
    pthread_t tid1, tid2, tid3;
    //create parent socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval , sizeof(int));

    //build the server's Internet address
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);
    // bind: associate the parent socket with a port
    if (bind(sockfd, (struct sockaddr *) &serveraddr,
             sizeof(serveraddr)) < 0)
        perror("ERROR on binding");
    clientlen = sizeof(clientaddr);
    //recvfrom: receive a UDP datagram from a client
    memset(buf, 0, LEN);
    n = recvfrom(sockfd, buf, LEN, 0,
                 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
        perror("ERROR in recvfrom");
    memcpy(&pkt, buf, sizeof(pkt));
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                          sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
        perror("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
        perror("ERROR on inet_ntoa\n");
    printf("Got conn request: (%d, %d, Nil)from client: %s (%s)\n",pkt.connid, pkt.seqnum,
           hostp->h_name, hostaddrp);
     // reply to conn req
    printf("replying to conn req\n");
    pkt.connid=rand();
    pkt.seqnum=0;
    sprintf((char*)pkt.payload, "");
    memcpy(buf,&pkt, sizeof(pkt));
    n = sendto(sockfd, buf, sizeof(pkt), 0,
               (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
        perror("ERROR in sendto");
    //fill args to pass the threads;
    args info;
    info.id = pkt.connid;
    info.sock = sockfd;
    info.clientaddr = clientaddr;
    info.sent_data = 0;
    info.rcvd_data = 0;
    info.sent_ack = 0;
    info.rcvd_ack = 0;
    //pthread_create(&tid1, NULL, &send_thread, (void*)&info);
    pthread_create(&tid2, NULL, &recv_thread, (void*)&info);
    pthread_create(&tid3, NULL, &epoch_thread, (void*)&info);
    const char* msg = "ser hi";
    char s[10];
    info.outbox.enque(msg);info.outbox.enque(msg);info.outbox.enque(msg);info.outbox.enque(msg);
    sleep(10);
    info.inbox.deque(s);printf("From main: appln reads '%s'\n", s);
    info.inbox.deque(s);printf("From main: appln reads '%s'\n", s);
    info.inbox.deque(s);printf("From main: appln reads '%s'\n", s);
    info.inbox.deque(s);printf("From main: appln reads '%s'\n", s);
    /*
    while(true) {
        sleep(1);
        info.outbox.enque(msg);
        if(info.inbox.que_empty() != 0 && rand() %2 == 0 ){
            info.inbox.deque(s);
            printf("From main: appln reads '%s'\n", s);
        }
    }
    */
    //pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    return 0;
}
