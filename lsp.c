#include "lsp.h"
//#include "lspmessage.pb-c.h"
//#include "que.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define LEN 1000
char buf[LEN]; /*length of the buffer */

typedef struct
{
    uint32_t connid;
    uint32_t seqnum;
    uint8_t payload[50];
} lsp_packet;

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


/*
 *
 *
 *				CLIENT RELATED FUNCTIONS
 *
 *
 */

void *send_thread(void *a)
{
    lsp_client *info = (lsp_client *) a;
    int fd = info->sock;
    struct sockaddr_in serveraddr = info->serveraddr;
    socklen_t serverlen = sizeof (info->serveraddr);
    int n;
    // LSP message - using protobuf
    lsp_packet pkt;
    pkt.connid = info->id;
    pkt.seqnum = 1;
    while (1)
    {
        //sleep(10);
        if(info->seq_send > info->last_sent)
        {
            //todo protobuf
            pkt.seqnum = info->seq_send;
            sprintf((char*)pkt.payload, "x%d", pkt.seqnum);
            printf("Sending x%d\n", pkt.seqnum);
            //msg defn in common.h
            //Send a data for testing
            memset(buf, 0, LEN);
            memcpy(buf, &pkt, sizeof(pkt));
            n = sendto(fd, buf, sizeof(pkt), 0, (const sockaddr*)&serveraddr, serverlen);
            if(n < 0)
            {
                perror("ERROR Sending data send_thread");
            }
            else info->last_sent++;
        }
        //printf("Client send thread Last acked: %d, info->seq_recv: %d\n",info->seq_acked,info->seq_recv );
        if (info->seq_acked == info->seq_recv - 1  && rand()%2==0)
        {
            pkt.seqnum = info->seq_recv - 1;
            sprintf((char *) pkt.payload, "");
            memset(buf, 0, LEN);
            memcpy(buf, &pkt, sizeof (pkt));
            n = sendto(fd, buf, sizeof (pkt), 0, (const sockaddr*) &serveraddr, serverlen);
            if (n < 0)
            {
                perror("Error in sending ack");
            }
            else info->seq_acked++;
        }
    }
    return NULL;
}
void* recv_thread(void* a)
{
    int n;
    lsp_client* info = (lsp_client*) a;
    int fd = info->sock;
    struct sockaddr_in serveraddr = info->serveraddr;
    socklen_t serverlen = sizeof (serveraddr);
    char buf[LEN];
    lsp_packet pkt;
    memset(buf, 0, LEN);
    while (1)
    {
        n = recvfrom(fd, buf, sizeof (pkt), 0, (sockaddr*) & serveraddr, &serverlen);
        memcpy(&pkt, buf, sizeof(pkt));
        printf("Client rcv thread got Packet %d %d '%s'",pkt.connid, pkt.seqnum, pkt.payload);
        if (n < 0)
        {
            perror("ERROR Receiving in receiving thread\n");
        }
        if(pkt.payload[0] == '\0')
        {
            if(pkt.seqnum == info->seq_send)
            {
                printf("ACK recieved for message#%d\n", pkt.seqnum);
                info->seq_send++;
            }
            else printf("Duplicate ACK\n");
        }
        else
        {
            if(pkt.seqnum == info->seq_recv)
            {
                printf(" -> client rcv data packet #%d\n", info->seq_recv);
                info->seq_recv++;
            }
            else if(pkt.seqnum == info->seq_recv - 1)
            {
                printf(" -> client rcvd duplicate data packet#%d, must resend ACK\n", pkt.seqnum);
                info->seq_acked = pkt.seqnum;
            }
            else printf(" -> client rcvd out of order Server malfunctioning, sending \n");
        }
    }
    return NULL;
}

void* epoch_thread(void* a)
{
    lsp_client* info = (lsp_client*)a;
    int fd = info->sock;
    struct sockaddr_in serveraddr = info->serveraddr;
    socklen_t serverlen = sizeof(info->serveraddr);
    int n;
    lsp_packet pkt;
    pkt.connid = info->id;
    info->timeouts = 0;
    while(1)
    {
        sleep(2);
        if(info->seq_acked == info->seq_recv - 1 || info->seq_acked == info->seq_recv)
        {
            if(info->timeouts == 4)
                printf("No communications from server, disconnecting\n");
            pkt.seqnum = info->seq_recv - 1;
            sprintf((char *) pkt.payload, "");
            memset(buf, 0, LEN);
            memcpy(buf, &pkt, sizeof (pkt));
            printf("Timeout! from epoch handler Sending ack%d\n", pkt.seqnum);
            n = sendto(fd, buf, sizeof (pkt), 0, (const sockaddr*) &serveraddr, serverlen);
            if (n < 0)
            {
                perror("Error in sending ack");
            }
            else
            {
                if(info->seq_acked == info->seq_recv - 1)
                    info->seq_acked++;
                info->timeouts++;
            }
        }
        else info->timeouts = 0;
        if(info->seq_send > info->last_sent)
        {
            //todo protobuf
            pkt.seqnum = info->seq_send;
            sprintf((char*)pkt.payload, "x%d", pkt.seqnum);
            printf("Timeout! from epoch handler Sending x%d\n", pkt.seqnum);
            //msg defn in common.h
            //Send a data for testing
            memset(buf, 0, LEN);
            memcpy(buf, &pkt, sizeof(pkt));
            n = sendto(fd, buf, sizeof(pkt), 0, (const sockaddr*)&serveraddr, serverlen);
            if(n < 0)
            {
                perror("ERROR Sending data send_thread");
            }
            else info->last_sent++;
        }
    }
    return NULL;
}

lsp_client* lsp_client_create(const char* src, int port)
{
    lsp_client* a_client = new lsp_client();
    int sockfd, n;
    socklen_t serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");
    server = gethostbyname(src);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host as %s\n", src);
        exit(0);
    }
    /* build the server's Internet address */
    memset((char *) &serveraddr, 0, sizeof (serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *) server->h_addr,
          (char *) &serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port);

    memset(buf, 0, LEN);
    lsp_packet pkt;
    pkt.connid = 0;
    pkt.seqnum = 0;
    sprintf((char*)pkt.payload, "");
    memcpy(buf, &pkt, sizeof (pkt));

    /* send the message to the server */
    serverlen = sizeof (serveraddr);
    n = sendto(sockfd, buf, sizeof (pkt), 0, (const sockaddr*) &serveraddr, serverlen);
    if (n < 0)
        perror("ERROR in sendto");

    /* print the server's reply */
    memset(buf, 0, sizeof (buf));
    n = recvfrom(sockfd, buf, sizeof (pkt), 0, (sockaddr*) & serveraddr, &serverlen);
    if (n < 0)
        perror("ERROR in recvfrom");
    memcpy(&pkt, buf, sizeof (pkt));
    printf("Connection accepted, packet : %d, %d, '%s'\n", pkt.connid, pkt.seqnum, pkt.payload);
    a_client->id = pkt.connid;
    a_client->sock = sockfd;
    a_client->serveraddr = serveraddr;
    a_client->seq_send = 1;
    a_client->seq_recv = 1;
    a_client->seq_acked = 1;
    a_client->last_sent = 0;

    pthread_create(&(a_client->tid1), NULL, &send_thread, (void*) a_client);
    pthread_create(&(a_client->tid2), NULL, &recv_thread, (void*) a_client);
    pthread_create(&(a_client->tid3), NULL, &epoch_thread,(void*) a_client);
    return a_client;
}

int lsp_client_read(lsp_client* a_client, uint8_t* pld)
{
    while(a_client->inbox.peek((char*)pld) == 0);
    a_client->inbox.deque((char*)pld);
    return 1;
}

bool lsp_client_write(lsp_client* a_client, uint8_t* pld, int lth)
{
    if(a_client->outbox.enque((char*)pld))return true;
    else return false;
}

bool lsp_client_close(lsp_client* a_client)
{
    pthread_join(a_client->tid1, NULL);
    pthread_join(a_client->tid2, NULL);
    pthread_join(a_client->tid3, NULL);
}

/*
 *
 *
 *				SERVER RELATED FUNCTIONS
 *
 *
 */


lsp_server* lsp_server_create(int port)
{

}

int lsp_server_read(lsp_server* a_srv, void* pld, uint32_t* conn_id)
{

}

bool lsp_server_write(lsp_server* a_srv, void* pld, int lth, uint32_t conn_id)
{

}

bool lsp_server_close(lsp_server* a_srv, uint32_t conn_id)
{

}

int main()
{
    lsp_client* clip = lsp_client_create("127.0.0.1", 2700);
    lsp_client_close(clip);
    return 0;
}

