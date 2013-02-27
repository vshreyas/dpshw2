#include "lsp_client.h"
#include <cstdio>
#define MAX_LEN 8

int main(int argc, char* argv[])
{

    char payload[100];
    if(argc < 3) {
        fprintf(stderr, "run as ./request lower upper\n");
        exit(0);
    }
    char* hash = argv[1];;
    char* lower = argv[2];
    char* upper = argv[3];
    if(strlen(lower) > MAX_LEN || strlen(upper) > MAX_LEN) {
        fprintf(stderr, "Sorry, the range is too long for the current version.\n");
        exit(0);
    }
    lsp_client* clip = lsp_client_create("127.0.0.1", 2700);
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
            printf("Not found\n");
    }
    else printf("Disconnected\n");
    fflush(stdout);
    lsp_client_close(clip);
    return 0;
}
