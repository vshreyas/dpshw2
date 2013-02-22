#include "lsp_client.h"
#include <cstdio>

int main()
{
    lsp_client* clip = lsp_client_create("127.0.0.1", 2700);
    const char* msg = "cli hi";
    char s[15];
    //lsp_client_write(clip, (uint8_t*)msg, 3);lsp_client_write(clip, (uint8_t*)msg, 3);lsp_client_write(clip, (uint8_t*)msg, 3);
    lsp_client_write(clip, (uint8_t*)msg, 3);lsp_client_write(clip, (uint8_t*)msg, 3);lsp_client_write(clip, (uint8_t*)msg, 3);
    sleep(10);
    while(lsp_client_read(clip, (uint8_t*)s) > 0){
        printf("From main: appln reads '%s'\n", s);
    }
    lsp_client_close(clip);
    return 0;
}
