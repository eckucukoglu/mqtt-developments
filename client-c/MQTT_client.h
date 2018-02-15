#ifndef _MQTTCLIENT_H_
#define _MQTTCLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include "MQTTAsync.h"

#define NUMBER_OF_CONCURRENT_THREADS 5
#define NUMBER_OF_CONNECTION_PER_THREAD 3
// #define NUMBER_OF_PUBLISH_PER_CONNECTION 2

#define ADDRESS "tcp://195.87.203.80:1049"
// #define ADDRESS "tcp://10.106.231.44:1883"
#define CLIENTIDPREFIX "client_"

#define TOPIC "OSMAN"
#define QOS 1
#define TIMEOUT 100000L
#define KEEP_ALIVE_INTERVAL 60


#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0);

typedef struct thread_info {
    pthread_t thread;
    unsigned int internal_id;
    MQTTAsync client;
} thread_info;



long long int get_time_usec();
void get_client_id(char*, int);
void generate_payload(char*);
void connlost(void *, char *);
int msgarrvd(void *, char *, int, MQTTAsync_message *);
void onDisconnect(void*, MQTTAsync_successData*);
void onSend(void*, MQTTAsync_successData*);
void onConnectFailure(void*, MQTTAsync_failureData*);


volatile MQTTAsync_token deliveredtoken;
int connection_counter_per_thread[NUMBER_OF_CONCURRENT_THREADS];
int connection_finished[NUMBER_OF_CONCURRENT_THREADS];
// int publish_counter_per_connection[NUMBER_OF_CONCURRENT_THREADS];
// int publish_finished[NUMBER_OF_CONCURRENT_THREADS];

#endif  /* not defined _MQTTCLIENT_H_ */
