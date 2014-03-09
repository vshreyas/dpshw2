/*
 * File:   Queue.h
 * Author: JEYENTH
 *
 * Created on February 15, 2013, 1:04 PM
 */

#ifndef QUE_H
#define	QUE_H
#include<string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;

class que {
    char payload [1000][100];
    int tail;
    int head;
    pthread_mutex_t mutex1;
public:
    que()
    {
        tail=head=1;
        pthread_mutex_init(&mutex1, NULL);
    }
    ~que()
    {
        tail=head=0;
        pthread_mutex_destroy(&mutex1);
    }

    int que_full() {
        int i=1;
        if (head == tail + 1)
            i=0;
        return i;
    }

    int que_empty() {
        pthread_mutex_lock(&mutex1);
        int i=1;
        if (head == tail)
            i=0;
        pthread_mutex_unlock(&mutex1);
        return i;
    }

    int enque(const char *message) {
        int n;
        pthread_mutex_lock(&mutex1);
        n = que_full();
        if (n == 0) {
            pthread_mutex_unlock(&mutex1);
            return 0;
        }
        if (tail == 10000) {
            strcpy(payload[tail], message);
            tail = 1;
            pthread_mutex_unlock(&mutex1);
            return 1;
        }
        else {
            strcpy(payload[tail], message);
            tail++;
            pthread_mutex_unlock(&mutex1);
            return 1;
        }
    }

    int deque(char *message) {
        int n = 0;
        pthread_mutex_lock(&mutex1);
        n = que_empty();
        if (n == 0) {
            pthread_mutex_unlock(&mutex1);
            return 0;
        } else {
            strcpy(message,payload[head]);
            strcpy(payload[head],"");
            if (head == 10000) {
                head = 1;
            } else
                head++;
        }
        pthread_mutex_unlock(&mutex1);
        return 1;
    }

    int peek(char *message)
    {
        int n=1;
        pthread_mutex_lock(&mutex1);
        n=que_empty();
        if(n==0)
        {
            pthread_mutex_unlock(&mutex1);
            return 0;
        }
        else strcpy(message,payload[head]);
        pthread_mutex_unlock(&mutex1);
        return 1;
    }

};


#endif	/* QUE_H */

