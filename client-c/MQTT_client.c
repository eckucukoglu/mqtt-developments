#include "MQTT_client.h"

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
    int i;
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = message->payload;
    for(i = 0; i < message->payloadlen; i++) {
        putchar(*payloadptr++);
    }
    putchar('\n');
    
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
    // int id = tinfo->internal_id;
    int rc;
#ifdef DEBUG
    printf("Message with token value %d delivery confirmed\n", response->token);
#endif
    // publish_counter_per_connection[id]++;
    // publish_finished[id] = 1;

    // if (publish_counter_per_connection[id] >= NUMBER_OF_PUBLISH_PER_CONNECTION) {
        MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
        opts.onSuccess = onDisconnect;
        opts.context = context;

        if ((rc = MQTTAsync_disconnect(tinfo->client, &opts)) != MQTTASYNC_SUCCESS) {
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
