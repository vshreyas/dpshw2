
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
#include <map>
#include "lsp.h"
#include "que.h"
#include <iostream>
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
    //map<int, lsp_client> info;
    int fd;
} args;

map<int, lsp_client> info;

void dump(std::map<int, lsp_client> const& m)
{
    for(std::map<int, lsp_client>::const_iterator i(m.begin()), j(m.end());
i != j; ++i)
        std::cout << "[" << i->first << "] = " << "{id "<< i->second.id <<",sock "
            << i->second.sock <<",sent_data "<<i->second.sent_data<<", rcvd_data"<< i->second.rcvd_data <<", sent_ack"<<i->second.sent_ack<<", rcvd_ack"<<i->second.rcvd_ack<<'\n';
}

pthread_mutex_t lock_info;

void* send_thread(void* a)
{
    /*
       lsp_client* info = (lsp_client* )a;
       int fd = info->sock;
       struct sockaddr_in clientaddr = info->addr;
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
       }
    return NULL;
    */
}

void* recv_thread(void* a)
{
    int n;
    struct hostent *hostp; /* client host info */
    char *hostaddrp; /* dotted decimal host addr string */
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    char buf[LEN];
    memset(buf, 0, LEN);
    lsp_packet pkt;
    int fd = ((args*)a)->fd;
    while (1)
    {
        n = recvfrom(fd, buf, sizeof (pkt), 0, (sockaddr*) &clientaddr, &clientlen);
        memcpy(&pkt, buf, sizeof(pkt));
        printf("Server rcv thread got Packet %d %d '%s'",pkt.connid, pkt.seqnum, pkt.payload);
        map<int, lsp_client>::iterator it = info.find(pkt.connid);
        if(it!= info.end()) {
            it->second.timeouts = 0;
        }
        if (n < 0)
        {
            perror("ERROR Receiving in receiving thread\n");
        }
        if(pkt.payload[0] == '\0')
        {
            if(pkt.connid == 0)
            {
                if(pkt.seqnum == 0 && pkt.payload[0] == '\0')
                {
                    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                                          sizeof(clientaddr.sin_addr.s_addr), AF_INET);
                    if (hostp == NULL)
                        perror("ERROR on gethostbyaddr");
                    hostaddrp = inet_ntoa(clientaddr.sin_addr);
                    if (hostaddrp == NULL)
                        perror("ERROR on inet_ntoa\n");
                    printf("Got conn request: (%d, %d, Nil)from client: %s (%s) on port %u\n",pkt.connid, pkt.seqnum,
                           hostp->h_name, hostaddrp, ntohs(clientaddr.sin_port));
                    lsp_client cli;
                    cli.id = 0;
                    cli.sock = fd;
                    memcpy(&cli.addr, &clientaddr, clientlen);
                    cli.sent_data = 0;
                    cli.rcvd_data = 0;
                    cli.sent_ack = 0;
                    cli.rcvd_ack = -1;
                    *it.insert(pair<int, lsp_client>((int)cli.id, cli));

                }
                else fprintf(stderr, "Bad connection request");
            }
            else
            {
                if(pkt.seqnum == *it.sent_data)
                {
                    printf("ACK recieved for message#%d\n", pkt.seqnum);
                    *it.outbox.deque((char*)pkt.payload) ;
                    if(*it.rcvd_ack == *it.sent_data - 1)*it.rcvd_ack++;
                }
                else printf("Duplicate ACK\n");
            }
        }
        else
        {
            if(pkt.seqnum == *it.sent_ack + 1)
            {
                printf(" -> server rcv data packet #%d\n", pkt.seqnum);
                //pthread_mutex_
                if(*it.inbox.enque((char*)pkt.payload))
                    *it.rcvd_data = *it.sent_ack + 1;
                else fprintf(stderr, "Queue full");
                //pthread_mutex_
            }
            else if(pkt.seqnum == *it.sent_ack)
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
    int n;
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    lsp_packet pkt;
    int fd = ((args*)a)->fd;
    while(1)
    {
        sleep(2);
        map<int, lsp_client>::iterator it;
        printf("Map contains %d entries\n", info.size());
        for(it = info.begin(); it != info.end(); ++it)
        {
            it->second.timeouts++;
            memcpy(&clientaddr, &it->second.addr, sizeof(clientaddr));
            clientlen = sizeof(clientaddr);
            struct hostent *hostp; /* client host info */
            char *hostaddrp; /* dotted decimal host addr string */
            hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                                  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
            if (hostp == NULL)
                perror("ERROR on gethostbyaddr");
            hostaddrp = inet_ntoa(clientaddr.sin_addr);
            if (hostaddrp == NULL)
                perror("ERROR on inet_ntoa\n");
            printf("Replying to client: %d, %s (%s) on port %u\n",it->second.id,
                   hostp->h_name, hostaddrp, ntohs(it->second.addr.sin_port));
            if(it->second.id == 0)
            {
                // reply to conn req
                printf("replying to conn req\n");
                pkt.connid=rand();
                pkt.seqnum=0;
                sprintf((char*)pkt.payload, "");
                memcpy(buf,&pkt, sizeof(pkt));
                n = sendto(fd, buf, sizeof(pkt), 0,
                           (struct sockaddr *) &clientaddr, clientlen);
                if (n < 0)
                    perror("ERROR in sendto");
                lsp_client cli;
                memcpy(&cli, &(it->second), sizeof(lsp_client));
                cli.id = pkt.connid;
                info.insert(pair<int, lsp_client>(pkt.connid, cli));
                info.erase(0);
            }
            else
            {
                if(it->second.sent_ack == it->second.rcvd_data - 1 || it->second.sent_ack == it->second.rcvd_data)
                {
                    if(it->second.timeouts == 5)
                    {
                        printf("No communications from client, disconnecting\n");
                        close(it->second.sock);
                        info.erase(it);
                        //exit(0);
                    }
                    pkt.seqnum = it->second.rcvd_data;
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
                        it->second.sent_ack = it->second.rcvd_data;
                    }
                }

                if(it->second.sent_data == it->second.rcvd_ack || it->second.sent_data == it->second.rcvd_ack + 1)
                {
                    //todo protobuf
                    pkt.seqnum = it->second.rcvd_ack + 1;
                    //
                    int rv = it->second.outbox.que_empty();
                    //sprintf((char*)pkt.payload, "x%d", pkt.seqnum);
                    if(rv !=0)
                    {
                        it->second.outbox.peek((char*)pkt.payload);
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
                            it->second.sent_data = it->second.rcvd_ack + 1;
                        }
                    }
                }
            }
        }
        sleep(2);
    }
    return NULL;
}

int main()
{
    int sockfd; /* socket */
    int portno; /* port to listen on */
    struct sockaddr_in serveraddr; /* server's addr */
    int optval; /* flag value for setsockopt */
    portno = 2700;
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
    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        perror("ERROR on binding");
    args ar;
    ar.fd = sockfd;
    pthread_create(&tid2, NULL, &recv_thread, (void*)&ar);
    pthread_create(&tid3, NULL, &epoch_thread, (void*)&ar);
    const char* msg = "ser hi";
    char s[10];
    while(info.size() == 0)usleep(10);
    info.begin()->second.outbox.enque(msg);
    info.begin()->second.outbox.enque(msg);
    info.begin()->second.outbox.enque(msg);
    info.begin()->second.outbox.enque(msg);
    sleep(10);
    while(info.begin()->second.inbox.deque(s) > 0)
    {
        printf("From main: appln reads '%s'\n", s);
    }
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    return 0;
}
