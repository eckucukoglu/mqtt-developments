#include "MQTT_client.h"

int allocate_globals(int isSubscriber) {
    if (!(message_counter = malloc(sizeof(int) * number_of_concurrent_threads)))
        return -1;

    if (!(connection_counter_per_thread = malloc(sizeof(int) * number_of_concurrent_threads)))
        return -1;

    if (!(connection_finished = malloc(sizeof(int) * number_of_concurrent_threads)))
        return -1;

    if (isSubscriber) {
        if (!(message_transmission_latency = malloc(sizeof(double) * number_of_concurrent_threads)))
            return -1;

        if (!(disc_finished = malloc(sizeof(int) * number_of_concurrent_threads)))
            return -1;

        if (!(subscribed = malloc(sizeof(int) * number_of_concurrent_threads)))
            return -1;
    }

    return 0;
}

void free_globals(int isSubscriber) {
    if (message_counter != NULL)
        free(message_counter);

    if (connection_counter_per_thread != NULL)
        free(connection_counter_per_thread);

    if (connection_finished != NULL)
        free(connection_finished);

    if (isSubscriber) {
        if (message_transmission_latency != NULL)
            free(message_transmission_latency);

        if (disc_finished != NULL)
            free(disc_finished);

        if (subscribed != NULL)
            free(subscribed);
    }
}

void set_common_fields() {
    memset(message_counter, 0, number_of_concurrent_threads * sizeof(int));
    memset(connection_counter_per_thread, 0, number_of_concurrent_threads * sizeof(int));
    memset(connection_finished, 0, number_of_concurrent_threads * sizeof(int));
}

void write_to_file(char* filename, char* content) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }

    fprintf(f, "%s\n", content);
    fclose(f);
}

long long int get_time_usec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long int ts_usec = ((long long int) tv.tv_sec) * 1000000ll +
        (long long int) tv.tv_usec;

    return ts_usec;
}

void get_client_id(char* client_id, int internal_id) {
    char ts_usec_str[20];
    sprintf(ts_usec_str, "%lld", get_time_usec());
    char id_str[3];
    sprintf(id_str, "%d", internal_id);
    strcpy(client_id, CLIENTIDPREFIX);
    strcat(client_id, ts_usec_str);
    strcat(client_id, "_");
    strcat(client_id, id_str);
}

void generate_payload(char* payload) {
    sprintf(payload, "%lld", get_time_usec());
}

void connlost(void *context, char *cause) {
    thread_info *tinfo = context;
    int id = tinfo->internal_id;

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
    printf("Reconnecting\n");
    conn_opts.keepAliveInterval = KEEP_ALIVE_INTERVAL;
    conn_opts.cleansession = 1;
    if ((rc = MQTTAsync_connect(tinfo->client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start connect, return code %d\n", rc);
        connection_finished[id] = 1;
    }
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    long long int receive_time_usec = get_time_usec();
    thread_info *tinfo = context;
    int id = tinfo->internal_id;
    int i;
    char* payloadptr;

    char message_str[20] = {0};
    payloadptr = message->payload;

    for(i = 0; i < message->payloadlen; i++) {
        message_str[i] = *payloadptr++;
    }
    message_str[i] = '\0';

    long long int sent_time_usec = atoll(message_str);
    long long int latency_usec = receive_time_usec - sent_time_usec;
    double latency_msec = latency_usec / 1000.0;
#ifdef DEBUG
    printf("Message arrived\n");
    printf("     topic: %s\tmessage: ", topicName);
    printf("%s\t", message_str);
    printf("latency(ms): %f\n", latency_msec);
#endif
    message_transmission_latency[id] += latency_msec;
    message_counter[id]++;

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response) {
    thread_info *tinfo = context;
    int id = tinfo->internal_id;
#ifdef DEBUG
    printf("Successful disconnection\n");
#endif
    connection_finished[id] = 1;
    connection_counter_per_thread[id]++;
}

void onSend(void* context, MQTTAsync_successData* response) {
    thread_info *tinfo = context;
    int id = tinfo->internal_id;
    int rc;
#ifdef DEBUG
    printf("Message with token value %d delivery confirmed\n", response->token);
#endif
    message_counter[id]++;
    // publish_counter_per_connection[id]++;
    // publish_finished[id] = 1;

    // if (publish_counter_per_connection[id] >= NUMBER_OF_PUBLISH_PER_CONNECTION) {
        MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
        opts.onSuccess = onDisconnect;
        opts.context = context;

        rc = MQTTAsync_disconnect(tinfo->client, &opts);
        if (rc != MQTTASYNC_SUCCESS) {
            printf("Failed to start sendMessage, return code %d\n", rc);
            exit(EXIT_FAILURE);
        }
    // }
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    thread_info *tinfo = context;
    int id = tinfo->internal_id;
    printf("Connect failed, rc %d\n", response ? response->code : 0);
    connection_finished[id] = 1;
}
