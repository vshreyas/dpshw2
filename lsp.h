#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <cstddef>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <cerrno>
#include <strings.h>
#include <cstring>
#include "que.h"

// Global Parameters. For both server and clients.

#define _EPOCH_LTH 2.0
#define _EPOCH_CNT 10;
#define _DROP_RATE 0.05;

void lsp_set_epoch_lth(double lth);
void lsp_set_epoch_cnt(int cnt);
void lsp_set_drop_rate(double rate);

typedef struct
{
    que inbox;
    que outbox;
    uint32_t id;
    int sock;
    struct sockaddr_in addr;
    int sent_data;
    int sent_ack;
    int rcvd_data;
    int rcvd_ack;
    pthread_t tid1, tid2, tid3;
    int timeouts;
    bool alive;
} lsp_client;

lsp_client* lsp_client_create(const char* dest, int port);
int lsp_client_read(lsp_client* a_client, uint8_t* pld);
bool lsp_client_write(lsp_client* a_client, uint8_t* pld, int lth);
bool lsp_client_close(lsp_client* a_client);


typedef struct
{
    int fd;
    pthread_t tid1;
    pthread_t tid2;
    pthread_t tid3;
} lsp_server;


lsp_server* lsp_server_create(int port);
int  lsp_server_read(lsp_server* a_srv, void* pld, uint32_t* conn_id);
bool lsp_server_write(lsp_server* a_srv, void* pld, int lth, uint32_t conn_id);
bool lsp_server_close(lsp_server* a_srv, uint32_t conn_id);
