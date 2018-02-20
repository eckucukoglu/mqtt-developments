#include "MQTT_client.h"

void set_fields() {
    memset(disc_finished, 0, number_of_concurrent_threads * sizeof(int));
    memset(subscribed, 0, number_of_concurrent_threads * sizeof(int));
    memset(message_transmission_latency, 0, number_of_concurrent_threads * sizeof(double));
}

void write_subscriber_info() {
    int total_sent_message = 0;
    for (int i = 0; i < number_of_concurrent_threads; i++) {
        total_sent_message += message_counter[i];
    }

    double total_msg_tranmission_latency = 0.0;
    for (int i = 0; i < number_of_concurrent_threads; i++) {
        total_msg_tranmission_latency += message_transmission_latency[i];
    }

    double avg_msg_transmission_latency =
        total_msg_tranmission_latency / total_sent_message;

    char filename[15];
    strcpy(filename, "results.sub");

    char content[1000];
    sprintf(content, "Concurrent threads: %d\n"
        "Total received message: %d\n"
        "Average received message: %f\n"
        "Average message transmission latency(ms): %f\n",
        number_of_concurrent_threads,
        total_sent_message,
        (double)total_sent_message/number_of_concurrent_threads,
        avg_msg_transmission_latency);

    write_to_file(filename, content);
}

void  signal_handler(int sig) {
     write_subscriber_info();
     exit(0);
}

void subscriber_onDisconnect(void* context, MQTTAsync_successData* response) {
    thread_info *tinfo = context;
    int id = tinfo->internal_id;
    printf("Successful disconnection\n");
    write_subscriber_info();
    disc_finished[id] = 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response) {
    thread_info *tinfo = context;
    int id = tinfo->internal_id;
#ifdef DEBUG
    printf("Subscribe succeeded\n");
#endif
    subscribed[id] = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    thread_info *tinfo = context;
    int id = tinfo->internal_id;
    printf("Subscribe failed, rc %d\n", response ? response->code : 0);
    connection_finished[id] = 1;
}

void subscriber_onConnect(void* context, MQTTAsync_successData* response) {
    thread_info *tinfo = context;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc;
#ifdef DEBUG
    printf("Successful connection\n");
    printf("Subscribing to topic %s using QoS%d \n\n", topic, qos);
#endif
    opts.onSuccess = onSubscribe;
    opts.onFailure = onSubscribeFailure;
    opts.context = context;
    deliveredtoken = 0;

    rc = MQTTAsync_subscribe(tinfo->client, topic, qos, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
}

void *subscriber_handler(void *targs) {
    thread_info *tinfo = targs;
    int id = tinfo->internal_id;
    int rc;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

    char client_id[40];
    get_client_id(client_id, id);
    rc = MQTTAsync_create(&(tinfo->client), ADDRESS, client_id,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to create client, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    rc = MQTTAsync_setCallbacks(tinfo->client, tinfo, connlost, msgarrvd, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to set callbacks.\n");
        exit(EXIT_FAILURE);
    }

    conn_opts.keepAliveInterval = KEEP_ALIVE_INTERVAL;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = subscriber_onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = tinfo;

    rc = MQTTAsync_connect(tinfo->client, &conn_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to start connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    while (subscribed[id] == 0)
        usleep(TIMEOUT);

    if (connection_finished[id] == 1)
        goto exit;

    sleep(KEEP_ALIVE_INTERVAL);
    // int ch;
    // do {
    //     ch = getchar();
    // } while (ch!='Q' && ch != 'q');

    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    disc_opts.onSuccess = subscriber_onDisconnect;
    disc_opts.context = tinfo;

    rc = MQTTAsync_disconnect(tinfo->client, &disc_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to start disconnect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    while (disc_finished[id] == 0)
        usleep(TIMEOUT);
exit:
    MQTTAsync_destroy(&(tinfo->client));

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        return -1;
    }

    thread_info *tinfo;
    void *res;
    int rc;

    signal(SIGINT, signal_handler);
    char pid[7];
    sprintf(pid, "%d", getpid());
    write_to_file("pids.sub", pid);

    number_of_concurrent_threads = atoi(argv[1]);
    qos = atoi(argv[2]);
    strcpy(topic, argv[3]);

    printf("pid:%d\tthreads:%d\tQoS:%d\ttopic:%s\n",
        getpid(), number_of_concurrent_threads, qos, topic);

    if (allocate_globals(1) != 0) {
        printf("memory allocation error!\n");
        return -1;
    }
    set_common_fields();
    set_fields();

    tinfo = malloc(sizeof(thread_info) * number_of_concurrent_threads);
    for (int i = 0; i < number_of_concurrent_threads; i++) {
        tinfo[i].internal_id = i;

        rc = pthread_create(&tinfo[i].thread, NULL,
                            subscriber_handler, (void*)&tinfo[i]);
        if (rc) {
            handle_error_en(rc, "pthread_create");
        }
    }
    sleep(1);

    for (int i = 0; i < number_of_concurrent_threads; i++) {
        rc = pthread_join(tinfo[i].thread, &res);

        if (rc) {
            handle_error_en(rc, "pthread_join");
        }

        free(res);
    }

    free(tinfo);
    free_globals(1);
    exit(EXIT_SUCCESS);
}
