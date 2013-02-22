#include "lsp_server.h"
#include <queue>
#define LEN 1024
using namespace std;

int main(int argc, char** argv)
{
	lsp_server* myserver = lsp_server_create(2700);
	char pld[LEN];
	int connid;
	while(true) {
        lsp_server_read(myserver, pld, (uint32_t*)&connid);
        printf("Server got message %s from %d, replying.\n", s, connid);
	}
	lsp_server_close(myserver, connid);
	return 0;
}
