#include "MQTT_client.h"

// int publish_counter_per_connection[NUMBER_OF_CONCURRENT_THREADS];
// int publish_finished[NUMBER_OF_CONCURRENT_THREADS];

void set_fields() {
    // memset(publish_counter_per_connection, 0, sizeof(publish_counter_per_connection));
    // memset(publish_finished, 0, sizeof(publish_finished));
}

void write_publisher_info() {
    int total_sent_message = 0;
    for (int i = 0; i < number_of_concurrent_threads; i++) {
        total_sent_message += message_counter[i];
    }

    int pid = getpid();
    char pid_str[7];
    sprintf(pid_str, "%d", pid);

    char filename[15];
    strcpy(filename, pid_str);
    strcat(filename, ".pub");

    char content[1000];
    sprintf(content, "Concurrent threads: %d\n"
        "Connection per thread: %d\n"
        "Total sent message: %d\n",
        number_of_concurrent_threads,
        number_of_connection_per_thread,
        total_sent_message);

    write_to_file(filename, content);
}

void publisher_onConnect(void* context, MQTTAsync_successData* response) {
    thread_info *tinfo = context;
    // int id = tinfo->internal_id;
    int rc;

#ifdef DEBUG
    printf("Successful connection\n");
#endif

    // while (publish_counter_per_connection[id] < NUMBER_OF_PUBLISH_PER_CONNECTION) {
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        opts.onSuccess = onSend;
        opts.context = context;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        pubmsg.qos = qos;
        pubmsg.retained = 0;

        char payload[20];
        generate_payload(payload);
        pubmsg.payload = payload;
        pubmsg.payloadlen = strlen(payload);
        deliveredtoken = 0;
        rc = MQTTAsync_sendMessage(tinfo->client, TOPIC, &pubmsg, &opts);
        if (rc != MQTTASYNC_SUCCESS) {
            printf("Failed to start sendMessage, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }

    //     while (publish_finished[id] == 0) {
    //         sleep(5);
    //     }
    //
    //     publish_finished[id] = 0;
    // }
}

void *publisher_handler(void *targs) {
    thread_info *tinfo = targs;
    int id = tinfo->internal_id;
    int rc;

    while (connection_counter_per_thread[id] < number_of_connection_per_thread) {
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

        char client_id[40];
        get_client_id(client_id, id);

        rc = MQTTAsync_create(&(tinfo->client), ADDRESS, client_id,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
        if (rc != MQTTASYNC_SUCCESS) {
            printf("Failed to create client, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }

        rc = MQTTAsync_setCallbacks(tinfo->client, tinfo, connlost, NULL, NULL);
        // if (rc != MQTTASYNC_SUCCESS) {
        //     printf("Failed to set callbacks.\n");
        //     exit(EXIT_FAILURE);
        // }

        conn_opts.keepAliveInterval = KEEP_ALIVE_INTERVAL;
        conn_opts.cleansession = 1;
        conn_opts.onSuccess = publisher_onConnect;
        conn_opts.onFailure = onConnectFailure;
        conn_opts.context = tinfo;

        rc = MQTTAsync_connect(tinfo->client, &conn_opts);
        if (rc != MQTTASYNC_SUCCESS) {
            printf("Failed to start connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }

#ifdef DEBUG
        printf("Waiting for publication on topic %s for client with ClientID: %s\n",
            TOPIC, client_id);
#endif

        while (connection_finished[id] == 0) {
            usleep(timeout);
        }

        MQTTAsync_destroy(&(tinfo->client));
        connection_finished[id] = 0;
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        return -1;
    }

    thread_info *tinfo;
    void *res;
    int rc;

    number_of_concurrent_threads = atoi(argv[1]);
    number_of_connection_per_thread = atoi(argv[2]);
    qos = atoi(argv[3]);
    timeout = atol(argv[4]);
    printf("#threads: %d\t #con/thread: %d\tQoS: %d\ttimeout: %ld\n",
        number_of_concurrent_threads, number_of_connection_per_thread,
        qos, timeout);

    if (allocate_globals(0) != 0) {
        printf("memory allocation error!\n");
        return -1;
    }
    set_common_fields(number_of_concurrent_threads);
    set_fields();

    tinfo = malloc(sizeof(thread_info) * number_of_concurrent_threads);
    for (int i = 0; i < number_of_concurrent_threads; i++) {
        tinfo[i].internal_id = i;


        rc = pthread_create(&tinfo[i].thread, NULL,
                            publisher_handler, (void*)&tinfo[i]);
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

    write_publisher_info();

    free(tinfo);
    free_globals(0);
    exit(EXIT_SUCCESS);
}
