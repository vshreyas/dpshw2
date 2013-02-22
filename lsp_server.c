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

double epoch_lth = _EPOCH_LTH;
int epoch_cnt = _EPOCH_CNT;
double drop_rate = _DROP_RATE;
/*
 *
 *
 *				LSP RELATED FUNCTIONS
 *
 *
 */

void lsp_set_epoch_lth(double lth)
{
    epoch_lth = lth;
}
void lsp_set_epoch_cnt(int cnt)
{
    epoch_cnt = cnt;
}
void lsp_set_drop_rate(double rate)
{
    drop_rate = rate;
}
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
pthread_mutex_t lock_info;
/*
 *
 *
 *				SERVER RELATED FUNCTIONS
 *
 *
 */

void dump(std::map<int, lsp_client> const& m)
{
    for(std::map<int, lsp_client>::const_iterator i(m.begin()), j(m.end());
            i != j; ++i)
        std::cout << "[" << i->first << "] = " << "{id "<< i->second.id <<",sock "
                  << i->second.sock <<",sent_data "<<i->second.sent_data<<", rcvd_data"<< i->second.rcvd_data
                  <<", sent_ack"<<i->second.sent_ack<<", rcvd_ack "<<i->second.rcvd_ack<<"port "<< htons(i->second.addr.sin_port)<<"}\n";
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
    int fd = ((lsp_server*)a)->fd;
    while (1)
    {
        n = recvfrom(fd, buf, sizeof (pkt), 0, (sockaddr*) &clientaddr, &clientlen);
        if (n < 0)
        {
            perror("ERROR Receiving in receiving thread\n");
            continue;
        }
        memcpy(&pkt, buf, sizeof(pkt));
        //printf("Server rcv thread got Packet %d %d '%s'",pkt.connid, pkt.seqnum, pkt.payload);
        pthread_mutex_lock(&lock_info);
        map<int, lsp_client>::iterator it = info.find(pkt.connid);
        if(it!= info.end())
        {
            it->second.timeouts = 0;
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
                    bool duplicate = false;
                    for(map<int, lsp_client>::iterator tor = info.begin(); tor != info.end(); ++tor)
                    {
                        if(pkt.connid != tor->first)
                        {
                            if(memcmp(&(tor->second.addr), &clientaddr, clientlen) ==0)
                            {
                                duplicate = true;
                                //printf("Duplicate conn\n");
                            }
                        }
                    }
                    if(!duplicate)
                    {
                        lsp_client cli;
                        cli.id = 0;
                        cli.sock = fd;
                        memcpy(&cli.addr, &clientaddr, clientlen);
                        cli.sent_data = 0;
                        cli.rcvd_data = 0;
                        cli.sent_ack = 0;
                        cli.rcvd_ack = -1;
                        cli.timeouts = 0;
                        info.insert(pair<int, lsp_client>((int)cli.id, cli));
                        //printf("After new conn inserted by recv_thread: ");
                        dump(info);
                    }
                }
                else fprintf(stderr, "Bad connection request");
            }
            else
            {
                if(pkt.seqnum == it->second.sent_data)
                {
                    //printf("ACK recieved for message#%d\n", pkt.seqnum);
                    if(pkt.seqnum == it->second.rcvd_ack+1)it->second.outbox.deque((char*)pkt.payload) ;
                    if(it->second.rcvd_ack == it->second.sent_data - 1)it->second.rcvd_ack++;
                }
                else printf("Duplicate ACK\n");
            }
        }
        else
        {
            if(pkt.seqnum == it->second.sent_ack + 1)
            {
                //printf(" -> server rcv data packet #%d\n", pkt.seqnum);
                if(it->second.inbox.enque((char*)pkt.payload))
                    it->second.rcvd_data = it->second.sent_ack + 1;
                else fprintf(stderr, "Queue full");
            }
            else if(pkt.seqnum == it->second.sent_ack)
            {
                //printf(" -> server rcvd duplicate data packet#%d, must resend ACK\n", pkt.seqnum);
            }
            else printf(" -> server rcvd out of order Server malfunctioning, sending \n");
        }
        pthread_mutex_unlock(&lock_info);
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
    int fd = ((lsp_server*)a)->fd;
    while(1)
    {
        if(pthread_mutex_lock(&lock_info) < 0)perror("Mutex in epochthread");
        dump(info);
        map<int, lsp_client>::iterator it = info.begin();
        while(it != info.end())
        {
            if(it->second.timeouts == 5)
            {
                //printf("No communications from client, disconnecting\n");
                info.erase(it++);
                if(pthread_mutex_unlock(&lock_info) < 0)perror("Mutex in epochthread");

            }
            else
            {
                it->second.timeouts++;
                memcpy(&clientaddr, &(it->second.addr), sizeof(clientaddr));
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
                    //printf("replying to conn req\n");
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
                    //printf("After inserting: ");
                    dump(info);
                    info.erase(it++);
                    //printf("After erasing: ");
                    dump(info);
                }
                else
                {
                    if(it->second.sent_ack == it->second.rcvd_data - 1 || it->second.sent_ack == it->second.rcvd_data)
                    {
                        pkt.seqnum = it->second.rcvd_data;
                        sprintf((char *) pkt.payload, "");
                        memset(buf, 0, LEN);
                        memcpy(buf, &pkt, sizeof (pkt));
                        //printf("Timeout! from epoch handler Sending ack%d\n", pkt.seqnum);
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
                            //printf("Timeout! from epoch handler Sending %s %d\n", pkt.payload, pkt.seqnum);
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
                    ++it;
                }
            }
        }
        if(pthread_mutex_unlock(&lock_info) < 0)perror("Mutex in epochthread");
        sleep(2);
    }
    return NULL;
}

/**
 *
 */
lsp_server* lsp_server_create(int port)
{
    int sockfd; //socket
    struct sockaddr_in serveraddr; // server's addr
    int optval; // flag value for setsockop
    lsp_server* serv = new lsp_server();
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
    serveraddr.sin_port = htons((unsigned short)port);
    // bind: associate the parent socket with a port
    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        perror("ERROR on binding");
    serv->fd = sockfd;
    pthread_mutex_init(&lock_info, NULL);
    pthread_create(&tid2, NULL, &recv_thread, (void*)serv);
    pthread_create(&tid3, NULL, &epoch_thread, (void*)serv);
    serv->tid2 = tid2;
    serv->tid3 = tid3;

    return serv;
}

/**
 *
 */

int lsp_server_read(lsp_server* a_srv, void* pld, uint32_t* conn_id)
{
    map<int, lsp_client>::iterator it;
    char* payload = (char*)pld;
    payload[0] = '\0';
    while(strlen(payload) == 0) {
        pthread_mutex_lock(&lock_info);
        for(it = info.begin();it != info.end();++it) {
            it->second.inbox.deque(payload);
            if(strlen(payload) != 0){
                *conn_id = it->first;
                break;
            }
        }
        pthread_mutex_unlock(&lock_info);
        usleep(100);
    }

    return strlen((char*)pld);
}

/**
 *
 */
bool lsp_server_write(lsp_server* a_srv, void* pld, int lth, uint32_t conn_id)
{
    bool success = false;
    pthread_mutex_lock(&lock_info);
    success = (info[conn_id].outbox.enque((const char*)pld) > 0);
    pthread_mutex_unlock(&lock_info);
    return success;
}

/**
 *
 */
bool lsp_server_close(lsp_server* a_srv, uint32_t conn_id)
{
    pthread_join(a_srv->tid2, NULL);
    pthread_join(a_srv->tid3, NULL);
    pthread_mutex_destroy(&lock_info);
    //close()
    delete a_srv;
    return true;
}
