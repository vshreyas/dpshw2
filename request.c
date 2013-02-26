#include "lsp_client.h"
#include <cstdio>

int main(int argc, char* argv[])
{
    lsp_client* clip = lsp_client_create("127.0.0.1", 2700);
    char payload[100];
    if(argc < 3) {
        fprintf(stderr, "run as ./request lower upper\n");
        exit(0);
    }
    char* hash = argv[1];;
    char* lower = argv[2];
    char* upper = argv[3];
    if(strlen(lower) > 5 || strlen(upper) > 5) {
        fprintf(stderr, "Sorry, the range is too long for the current version.\n");
        exit(0);
    }
    memset(payload, 0, 60);
    sprintf(payload, "c %s %s %s", hash, lower, upper);
    lsp_client_write(clip, (uint8_t*)payload, strlen(payload));
    char pass[20];
    lsp_client_read(clip, (uint8_t*)pass);
    printf("Cracked password %s", pass);
    lsp_client_close(clip);
    return 0;
}
