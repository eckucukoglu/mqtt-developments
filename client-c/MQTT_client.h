#ifndef _MQTTCLIENT_H_
#define _MQTTCLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include "MQTTAsync.h"

// #define NUMBER_OF_CONCURRENT_THREADS 10
// #define NUMBER_OF_CONNECTION_PER_THREAD 100
// #define QOS 0
#define TIMEOUT 100000L // 100.000L microsecond = 0.1 second
// #define NUMBER_OF_PUBLISH_PER_CONNECTION 2

// #define ADDRESS "tcp://195.87.203.80:1049"
#define ADDRESS "tcp://10.106.231.44:1883"
#define CLIENTIDPREFIX "client_"

// #define TOPIC "OSMAN"
#define KEEP_ALIVE_INTERVAL 6000


#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0);

typedef struct thread_info {
    pthread_t thread;
    unsigned int internal_id;
    MQTTAsync client;
} thread_info;

int allocate_globals(int);
void set_common_fields();
void write_to_file(char*, char*);
long long int get_time_usec();
void get_client_id(char*, int);
void generate_payload(char*);
void connlost(void *, char *);
int msgarrvd(void *, char *, int, MQTTAsync_message *);
void onDisconnect(void*, MQTTAsync_successData*);
void onSend(void*, MQTTAsync_successData*);
void onConnectFailure(void*, MQTTAsync_failureData*);
void free_globals(int);

int number_of_concurrent_threads;
int number_of_connection_per_thread;
int qos;
int interval;
char topic[10];

volatile MQTTAsync_token deliveredtoken;
int *message_counter;
int *connection_counter_per_thread;
int *connection_finished;

double *message_transmission_latency;
int *disc_finished;
int *subscribed;

#endif  /* not defined _MQTTCLIENT_H_ */
