#include "lsp_server.h"

int main(int argc, char** argv)
{
	lsp_server* myserver = lsp_server_create(2700);
	char s[50];
	int connid;
	while(true) {
        int bytes = lsp_server_read(myserver, s, (uint32_t*)&connid);
        if(bytes >0) {
            printf("Chat server got message %s from %d, replying.\n", s, connid);
            char msg[10] = "ser hi";
            strcat(s, msg);
            lsp_server_write(myserver, s, 3, (uint32_t)connid);
        }
        else {
            lsp_server_close(myserver, connid);
            printf("Connid %d left\n", connid);
        }
	}
	lsp_server_close(myserver, connid);
	return 0;
}
