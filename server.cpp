#include "lsp_server.h"
#include <cstdio>
#include <deque>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include <cmath>
#define LEN 1024
using namespace std;

class request
{
public:
    int connid;
    char hash[41];
    char lower[10];
    char higher[10];
    request(int id, char* hvalue, char* low, char* high)
    {
        connid = id;
        strcpy(hvalue, hash);
        strcpy(lower, low);
        strcpy(higher, high);
    }
    request() {};
};

deque<request> pending;
map<int, request> assigned;
set<int> available;
map<int, int> subranges;

void dump(deque<request> q)
{
    deque<request>::iterator it;
    printf("jobQ[");
    for(it = q.begin(); it !=q.end(); ++it)
    {
        printf("%d, ", it->connid);
    }
    printf("]\n");
}

void display(map<int, request> m)
{
    map<int, request>::iterator it;
    printf("assigned[");
    for(it = m.begin(); it !=m.end(); ++it)
    {
        printf("(%d,%d), ", it->first, it->second.connid);
    }
    printf("]\n");
}

void display(map<int, int> m)
{
    map<int, int>::iterator it;
    printf("subranges[");
    for(it = m.begin();it != m.end();++it)
    {
        printf("(%d,%d), ", it->first, it->second);
    }
    printf("]\n");
}

void dump()
{
    printf("freelist[");
    for(set<int>::iterator it = available.begin(); it != available.end(); ++it)
    {
        printf("%d, ", *it);
    }
    printf("]\n");
}

int convert(char *pass)
{
    int n = 0, sum = 0, j = 0;
    for (int i = strlen(pass) - 1; i >= 0; i--)
    {
        n = pow(26.0, j);
        sum += (pass[i] - 'a') * n;
        j++;
    }
    return sum;
}

string form_string(int k, int len)
{
    string init = "", fin = "";
    while (len--)
    {
        char ch = 'a' + (k % 26);
        init += ch;
        k = k / 26;
    }
    for (int j = init.size() - 1; j >= 0; j--)
    {
        fin += init[j];
    }
    return fin;


}

vector<request> divide_range(request r, int n)
{
    int start = convert(r.lower);
    int end = convert(r.higher);
    int width = ceil((end - start)/n);
    vector<request> v;
    request s;
    s.connid = r.connid;
    strcpy(s.hash, r.hash);
    while(start < end)
    {
        strcpy(s.lower, form_string(start, strlen(r.lower)).c_str());
        strcpy(s.higher, form_string(min(start+width, end), strlen(r.lower)).c_str());
        v.push_back(s);
        start += width;
    }
    return v;
}

int main(int argc, char** argv)
{
    int port;
    if(argc < 1) port = 2700;
    else port = atoi(argv[1]);
    lsp_server* myserver = lsp_server_create(port);
    char pld[LEN];
    int connid;
    int bytes = 0;
    request curr;

    while(true)
    {
        memset(pld, 0, LEN - 1);
        bytes = lsp_server_read(myserver, pld, (uint32_t*)&connid);
        if(bytes < 0)
        {
            printf("Lost connection with %d\n", connid);
            lsp_server_close(myserver, connid);
            available.erase(connid);
            if(assigned.find(connid) != assigned.end()) {
                pending.push_front(assigned[connid]);
            }
            assigned.erase(connid);
            subranges.erase(connid);
            dump();
            display(assigned);
        }
        //printf("Server got message %s from %d, replying.\n", pld, connid);
        else
        {
            if(pld[0]=='c')
            {
                printf("crack request got from client %d\n", connid);
                request r;
                r.connid = connid;
                char* tok = strtok(pld, " ");
                if(tok != NULL) tok = strtok(NULL, " ");
                strcpy(r.hash, tok);
                if(tok != NULL) tok = strtok(NULL, " ");
                strcpy(r.lower, tok);
                if(tok != NULL) tok = strtok(NULL, " ");
                strcpy(r.higher, tok);
                if(tok==NULL)fprintf(stderr, "Corrupt LSP pkt\n");
                sprintf(pld, "c %s %s %s", r.hash, r.lower, r.higher);

                vector<request> v = divide_range(r, 10);
                subranges.insert(pair<int,int>(r.connid, 11));
                printf("{");
                for(vector<request>::iterator i = v.begin(); i != v.end(); ++i)
                {
                    printf("[%s, %s]", i->lower, i->higher);
                    pending.push_back(*i);
                }
                printf("}\n");
                dump(pending);
                //try to assign the job at the front
                while(!available.empty())
                {
                    set<int>::iterator it = available.begin();
                    if(!pending.empty())
                    {
                        curr = pending.front();
                        int drone = *it;
                        assigned.insert(pair<int, request>(drone, curr));
                        sprintf(pld, "c %s %s %s", curr.hash, curr.lower, curr.higher);
                        printf("Assigned %s to %d\n", pld, drone);
                        lsp_server_write(myserver, pld, strlen(pld), drone);
                        printf("Before allocating:");
                        dump();
                        printf("Aftre allocating:");
                        available.erase(drone);
                        ++it;
                        dump();
                        display(assigned);
                        pending.pop_front();
                    }
                    else break;
                }
            }
            else if(pld[0]=='j')
            {
                printf("join request got from worker %d\n", connid);
                available.insert(connid);
                dump();
                //try to assign the job at the front of the request deque
                while(!available.empty())
                {
                    set<int>::iterator it = available.begin();
                    if(!pending.empty())
                    {
                        curr = pending.front();
                        for(set<int>::iterator it = available.begin(); it != available.end(); )
                        {
                            int drone = *it;
                            assigned.insert(pair<int, request>(drone, curr));
                            sprintf(pld, "c %s %s %s", curr.hash, curr.lower, curr.higher);
                            lsp_server_write(myserver, pld, strlen(pld), drone);
                            printf("Before allocating\n");
                            dump();
                            printf("Aftre allocating\n");
                            ++it;
                            available.erase(drone);
                            dump();
                            display(assigned);
                            pending.pop_front();
                        }
                    }
                    else break;
                }
            }
            else if(pld[0] == 'f')
            {
                printf("password found! by worker %d.", connid);
                if(assigned.find(connid) == assigned.end())
                {
                    fprintf(stderr, "unassigned worker zombie\n");
                    continue;
                }
                int cust = assigned[connid].connid;
                char pld1[LEN];
                strcpy(pld1, pld);
                char* tok = strtok(pld, " ");
                if(tok!=NULL) tok = strtok(NULL, " ");
                else
                {
                    printf("Corrupt/malformed LSP pkt\n");
                    continue;
                }
                if(tok != NULL) printf("it is %s.Sending it to cust%d\n", tok, cust);
                bool success = lsp_server_write(myserver, pld1, strlen(pld1), cust);
                if(success)
                {
                    lsp_server_close(myserver, cust);
                }
                else fprintf(stderr,"Error sending passwrod back to customer\n");
                assigned.erase(connid);
                available.insert(connid);
                dump();
                //pending.erase(cust);
                if(!pending.empty())
                {
                    set<int>::iterator it = available.begin();
                    while(!available.empty())
                    {
                        if(!pending.empty())
                        {
                            curr = pending.front();
                            int drone = *it;
                            assigned.insert(pair<int, request>(drone, curr));
                            sprintf(pld, "c %s %s %s", curr.hash, curr.lower, curr.higher);
                            lsp_server_write(myserver, pld, strlen(pld), drone);
                            printf("Before allocating\n");
                            dump();
                            printf("Aftre allocating\n");
                            ++it;
                            available.erase(drone);
                            dump();
                            display(assigned);
                            pending.pop_front();
                        }
                        else break;
                    }
                }
            }
            else if(pld[0] == 'x')
            {
                printf("not found says worker %d", connid);
                available.insert(connid);
                int cust;
                if(assigned.find(connid) != assigned.end()) {
                    cust = assigned[connid].connid;
                    printf("assigned to %d", cust);
                }
                else {
                    printf("unassigned zombie\n");
                    continue;
                }
                display(subranges);
                if(subranges.find(cust) != subranges.end()) {
                    --subranges[cust];
                    printf("# ranges left for %d are %d", cust, subranges[cust]);
                    if(subranges[cust] == 0) {
                        printf("No password for client %d\n", cust);
                        lsp_server_write(myserver, pld, strlen(pld), cust);
                        subranges.erase(cust);
                    }
                }
                assigned.erase(connid);
                set<int>::iterator it = available.begin();
                while(!available.empty())
                {
                    it = available.begin();
                    if(!pending.empty())
                    {
                        curr = pending.front();
                        int drone = *it;
                        assigned.insert(pair<int, request>(drone, curr));
                        sprintf(pld, "c %s %s %s", curr.hash, curr.lower, curr.higher);
                        lsp_server_write(myserver, pld, strlen(pld), drone);
                        printf("Before allocating\n");
                        dump();
                        printf("Aftre allocating\n");
                        ++it;
                        available.erase(drone);
                        dump();
                        display(assigned);
                        pending.pop_front();
                    }
                    else break;
                }
            }
            else printf("Corrupt/malformed LSP pkt\n");
        }
    }
    //lsp_server_cleanup(myserver);
    return 0;
}
