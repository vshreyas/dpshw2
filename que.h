/* 
 * File:   Queue.h
 * Author: JEYENTH
 *
 * Created on February 15, 2013, 1:04 PM
 */

#ifndef QUE_H
#define	QUE_H
#include<queue>
#include<string>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>


using namespace std;

class que {
    char payload [10000][50];
    int tail;
    int head;
public:
    que()
    {
        tail=head=1;
    }
    ~que()
    {
        tail=head=0;
    }

    int que_full() {
        int i=1;
        if (head == tail + 1)
            i=0;
        return i;
    }

    int que_empty() {
        int i=1;
        if (head == tail)
            i=0;
        return i;
    }

    int enque(char *message) {
        int n,j;
        n = que_full();
        if (n == 0) {

            return 0;
        } else
            if (tail == 10000) {
            strcpy(payload[tail], message);
          
             tail = 1;
             return 1;
        }
        if (tail != 10000) {
            strcpy(payload[tail], message);
            
            tail++;
            return 1;
        }
    }

    int deque(char *message) {
        int n = 0;
        n = que_empty();
        if (n == 0) {
           
            return 0;
        } else {
            strcpy(message,payload[head]);
            
            strcpy(payload[head],"");
            if (head == 10000) {
                head = 1;
            } else
                head++;
        }
        return 1;
    }
    
    int peek(char *message)
    {
        int n=1;
        n=que_empty();
        if(n==0)
        {
            return 0;
        }
        if(n!=0)
        {
            strcpy(message,payload[head]);
        }
        return 1;
    }

};


#endif	/* QUE_H */

