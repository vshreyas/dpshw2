#include "lsp_client.h"
#include <cstdio>
#define MAX_LEN 8

int main(int argc, char* argv[])
{
    char payload[100];
    char* host;
    int port;
    if(argc < 3)
    {
        fprintf(stderr, "Run as ./request ip:port hash len\n");
        exit(0);
    }
    host = strtok(argv[1], ":");
    if(host == NULL)
    {
        fprintf(stderr, "Run as ./request ip:port hash len\n");
        exit(0);
    }
    port = atoi(strtok(NULL, ":"));
    printf("host %s, port %d", host, port);
    char* hash = argv[2];
    int len = atoi(argv[3]);
    if(len > MAX_LEN) {
        fprintf(stderr, "Sorry, the range is too long for the current version.\n");
        exit(0);
    }
    char lower[MAX_LEN + 2];
    char upper[MAX_LEN + 2];
    int i;
    for(i = 0; i < len;i++)lower[i] = 'a';
    lower[i] = '\0';
    for(i = 0; i < len;i++)upper[i] = 'z';
    upper[i] = '\0';
    lsp_client* clip = lsp_client_create(host, port);
    memset(payload, 0, 60);
    sprintf(payload, "c %s %s %s", hash, lower, upper);
    lsp_client_write(clip, (uint8_t*)payload, strlen(payload));
    char pass[20];
    int bytes_read = lsp_client_read(clip, (uint8_t*)pass);
    funlockfile(stdout);
    if(bytes_read > 0) {
        if(pass[0] == 'f')
            printf("Found %s\n", pass + 2);
        else if(pass[0] == 'x')
            fprintf(stdout, "Not found\n");
    }
    else fprintf(stdout, "Disconnected\n");
    lsp_client_close(clip);
    return 0;
}
