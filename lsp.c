#include "lsp.h"
//#include "lsp.pb.h"
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
#include <cstddef>
#include <sys/un.h>
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
{/*
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

        //Sending data: if an ack received for prev_ackious message
        if(info->sent_data == info->rcvd_ack)
        {
            //todo protobuf
            pkt.seqnum = info->rcvd_ack + 1;
            //sprintf((char*)pkt.payload, "x%d", pkt.seqnum);
            int rv = info->outbox.peek((char*)pkt.payload);
            if(rv != 0) {
                printf("Sending %s%d\n", pkt.payload, pkt.seqnum);
                //msg defn in common.h
                //Send a data for testing
                memset(buf, 0, LEN);
                memcpy(buf, &pkt, sizeof(pkt));
                n = sendto(fd, buf, sizeof(pkt), 0, (const sockaddr*)&serveraddr, serverlen);
                if(n < 0)
                {
                    perror("ERROR Sending data send_thread");
                }
                else info->sent_data++;
            }
        }
        //Sending ack if received message after last ack
        if (info->sent_ack == info->rcvd_data - 1)
        {
            pkt.seqnum = info->rcvd_data;
            sprintf((char *) pkt.payload, "");
            memset(buf, 0, LEN);
            memcpy(buf, &pkt, sizeof (pkt));
            n = sendto(fd, buf, sizeof (pkt), 0, (const sockaddr*) &serveraddr, serverlen);
            if (n < 0)
            {
                perror("Error in sending ack");
            }
            else info->sent_ack++;
        }

    }*/
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
    while (true)
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
                printf(" -> client rcv data packet #%d\n", pkt.seqnum);
                //pthread_mutex_
                if(info->inbox.enque((char*)pkt.payload))
                    info->rcvd_data = info->sent_ack + 1;
                //pthread_mutex_
            }
            else if(pkt.seqnum == info->sent_ack)
            {
                printf(" -> client rcvd duplicate data packet#%d, must resend ACK\n", pkt.seqnum);
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
    bool idle;
    int prev_ack = 0;
    int prev_data = 0;

    while(true)
    {
        sleep(2);
        idle = true;
        if(info->sent_ack == info->rcvd_data - 1 || info->sent_ack == info->rcvd_data)
        {
            if(info->timeouts == 5){
                printf("No communications from server, disconnecting\n");
                close(info->sock);
                free(info);
                exit(0);
            }
            pkt.seqnum = info->rcvd_data;
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
                info->sent_ack = info->rcvd_data;
                if(info->sent_ack != prev_ack)idle = false;
                prev_ack = info->sent_ack;
            }
        }

        if(info->sent_data == info->rcvd_ack || info->sent_data == info->rcvd_ack + 1)
        {
            //todo protobuf
            pkt.seqnum = info->rcvd_ack + 1;
            //sprintf((char*)pkt.payload, "x%d", pkt.seqnum);
            int rv = info->outbox.peek((char*)pkt.payload);
            if(rv !=0) {
                printf("Timeout! from epoch handler Sending %s%d\n", pkt.payload, pkt.seqnum);
                //msg defn in common.h
                //Send a data for testing
                memset(buf, 0, LEN);
                memcpy(buf, &pkt, sizeof(pkt));
                n = sendto(fd, buf, sizeof(pkt), 0, (const sockaddr*)&serveraddr, serverlen);
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
    a_client->sent_data = 0;
    a_client->rcvd_data = 0;
    a_client->sent_ack = 0;
    a_client->rcvd_ack = 0;

    //pthread_create(&(a_client->tid1), NULL, &send_thread, (void*) a_client);
    pthread_create(&(a_client->tid2), NULL, &recv_thread, (void*) a_client);
    pthread_create(&(a_client->tid3), NULL, &epoch_thread,(void*) a_client);
    return a_client;
}

int lsp_client_read(lsp_client* a_client, uint8_t* pld)
{
    while(a_client->inbox.que_empty() == 0)usleep(10);
    return a_client->inbox.deque((char*)pld);
}

bool lsp_client_write(lsp_client* a_client, uint8_t* pld, int lth)
{
    if(a_client->outbox.enque((const char*)pld))return true;
    else return false;
}

bool lsp_client_close(lsp_client* a_client)
{
    //pthread_join(a_client->tid1, NULL);
    pthread_join(a_client->tid2, NULL);
    pthread_join(a_client->tid3, NULL);
    return true;
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
    const char* msg = "cli hi";
    char s[15];
    lsp_client_write(clip, (uint8_t*)msg, 3);lsp_client_write(clip, (uint8_t*)msg, 3);lsp_client_write(clip, (uint8_t*)msg, 3);
    sleep(10);
    while(lsp_client_read(clip, (uint8_t*)s) > 0){
        printf("From main: appln reads '%s'\n", s);
    }
    lsp_client_write(clip, (uint8_t*)s, 5);
    /*
    while(true) {
        sleep(1);
        lsp_client_write(clip, (uint8_t*)msg, 3);
        memset(s, 0, 10);
        if(rand() % 2 == 0) {
            lsp_client_read(clip, (uint8_t*)s);
            printf("From main: appln reads '%s'\n", s);
        }
    }
    */
    lsp_client_close(clip);
    return 0;
}

