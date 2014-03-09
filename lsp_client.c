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


typedef struct
{
    uint32_t connid;
    uint32_t seqnum;
    uint8_t payload[100];
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

pthread_mutex_t lock_info;

/*
uint8_t* message_encode(int conn_id,int seq_no,const char* payload,int& outlength)
{
    int len;
    LSPMessage msg = LSPMESSAGE__INIT;
    msg.connid = conn_id;
    msg.seqnum = seq_no;
    msg.payload.data = (uint8_t *)malloc(sizeof(uint8_t) * (strlen(payload)+1));
    msg.payload.len = strlen(payload)+1;
    memcpy(msg.payload.data, payload, (strlen(payload)+1)*sizeof(uint8_t));
    len = lspmessage__get_packed_size(&msg);
    //cout<<"len" <<len<<" strlen "<<strlen(payload);
    uint8_t* buffer = (uint8_t *)malloc(len);
    lspmessage__pack(&msg, buffer);
    outlength=len;
    free(msg.payload.data);
    return buffer;

}

int message_decode(int len,uint8_t* buf, lsp_packet& pkt)
{

    LSPMessage* msg=lspmessage__unpack(NULL, len, buf);
    pkt.conn_id=msg->connid;
    pkt.seq_no=msg->seqnum;
    memcpy(pkt.data,msg->payload.data,msg->payload.len);
    //cout<<" Conn_id "<<pkt.conn_id<<"\n";
    //cout<<" Seq Num "<<pkt.seq_no<<"\n";
    //cout<<" payload data"<<pkt.data<<"\n";
    //cout<<" payload len"<<msg->payload.len<<"\n";
    //if(pkt.data[0]=='\0')cout<<"true" ;else cout<<"false";
    return msg->payload.len;
}

*/


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
    struct sockaddr_in serveraddr = info->addr;
    socklen_t serverlen = sizeof (info->addr);
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
    struct sockaddr_in serveraddr;
    memcpy(&serveraddr, &(info->addr), sizeof(serveraddr));
    socklen_t serverlen = sizeof (serveraddr);
    char buf[LEN];
    lsp_packet pkt;
    memset(buf, 0, LEN);
    while (true)
    {
        n = recvfrom(fd, buf, sizeof (pkt), 0, (sockaddr*) & serveraddr, &serverlen);
        if ((rand()/(float)RAND_MAX) < drop_rate) continue;
        if(n < 0) break;
        memcpy(&pkt, buf, sizeof(pkt));
        printf("Client rcv thread got Packet %d %d '%s'",pkt.connid, pkt.seqnum, pkt.payload);
        if(pkt.seqnum == 0)
        {
            info->id = pkt.connid;
            info->rcvd_ack = 0;
        }
        else if(pkt.connid != info->id) {
            fprintf(stderr, "Wrong connid\n");
            break;
        }
        info->timeouts = 0;
        if(pkt.payload[0] == '\0')
        {
            if(pkt.seqnum == info->sent_data)
            {
                printf("ACK recieved for message#%d\n", pkt.seqnum);
                if(pkt.seqnum == info->rcvd_ack + 1)info->outbox.deque((char*)pkt.payload) ;
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
    char buf[LEN];
    lsp_client* info = (lsp_client*)a;
    int fd = info->sock;
    struct sockaddr_in serveraddr = info->addr;
    socklen_t serverlen = sizeof(info->addr);
    int n;
    lsp_packet pkt;
    //Send connection request
    int tries = 0;
    while(info->rcvd_ack < 0 && tries < epoch_cnt) {
        memset(buf, 0, LEN);
        pkt.connid = 0;
        pkt.seqnum = 0;
        sprintf((char*)pkt.payload, "");
        memcpy(buf, &pkt, sizeof (pkt));
        serverlen = sizeof (serveraddr);
        n = sendto(fd, buf, sizeof (pkt), 0, (const sockaddr*) &serveraddr, serverlen);
        if (n < 0) {
            perror("ERROR in sendto");
            //exit(0);
        }
        else printf("Sent a conn req\n");
        tries++;
        sleep(epoch_lth);
    }
    if(tries == epoch_cnt) {
        //printf("Disconnected\n");
        info->inbox.enque("Disconnected\n");
        //info->alive = false;
        return NULL;
    }
    pkt.connid = info->id;
    bool idle;
    int prev_ack = 0;
    int prev_data = 0;

    while(true)
    {
        sleep(epoch_lth);
        info->timeouts++;
        if(info->sent_ack == info->rcvd_data - 1 || info->sent_ack == info->rcvd_data)
        {
            if(info->timeouts == 5){
                printf("No communications from server, disconnecting\n");
                info->inbox.enque("Disconnected\n");
                //info->alive = false;
                break;
            }
            pkt.seqnum = info->rcvd_data;
            sprintf((char *) pkt.payload, "");
            memset(buf, 0, LEN);
            memcpy(buf, &pkt, sizeof (pkt));
            //printf("Timeout! from epoch handler Sending ack%d\n", pkt.seqnum);
            n = sendto(fd, buf, sizeof (pkt), 0, (const sockaddr*) &serveraddr, serverlen);
            if (n < 0)
            {
                perror("Error in sending ack");
            }
            else
            {
                info->sent_ack = info->rcvd_data;
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
                }
            }
        }
    }
    return NULL;
}

lsp_client* lsp_client_create(const char* src, int port)
{
    lsp_client* a_client = new lsp_client();
    int sockfd, n;
    char buf[LEN]; /*length of the buffer */
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

    a_client->sock = sockfd;
    memcpy(&(a_client->addr), &serveraddr, sizeof (serveraddr));
    a_client->sent_data = 0;
    a_client->rcvd_data = 0;
    a_client->sent_ack = 0;
    a_client->rcvd_ack = -1;
    a_client->alive = true;
    pthread_create(&(a_client->tid2), NULL, &epoch_thread, (void*) a_client);
    /*
    lsp_packet pkt;
    memset(buf, 0, sizeof (buf));
    n = recvfrom(sockfd, buf, sizeof (pkt), 0, (sockaddr*) & serveraddr, &serverlen);
    if (n < 0)
        perror("ERROR in recvfrom");
    memcpy(&pkt, buf, sizeof (pkt));
    printf("Connection accepted, packet : %d, %d, '%s'\n", pkt.connid, pkt.seqnum, pkt.payload);
    a_client->rcvd_ack = 0;
    //pthread_create(&(a_client->tid1), NULL, &send_thread, (void*) a_client);
    a_client->id = pkt.connid;
    */
    pthread_create(&(a_client->tid3), NULL, &recv_thread,(void*) a_client);
    return a_client;
}

int lsp_client_read(lsp_client* a_client, uint8_t* pld)
{
    while(a_client->inbox.que_empty() == 0 ) {
        usleep(10);
    }
    int rv = a_client->inbox.deque((char*)pld);
    if(strcmp((char*)pld, "Disconnected\n") == 0) return -1;
    else return rv;
}

bool lsp_client_write(lsp_client* a_client, uint8_t* pld, int lth)
{
    //if(!a_client->alive) return false;
    if(a_client->outbox.enque((const char*)pld))return true;
    else return false;
}

bool lsp_client_close(lsp_client* a_client)
{
    //pthread_join(a_client->tid1, NULL);
    //pthread_join(a_client->tid2, NULL);
    //pthread_join(a_client->tid3, NULL);
    close(a_client->sock);
    delete a_client;
    pthread_mutex_destroy(&lock_info);
    fflush(stdout);
    return true;
}

