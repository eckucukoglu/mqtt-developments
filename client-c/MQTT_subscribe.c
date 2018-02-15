#include "MQTT_client.h"

int disc_finished = 0;
int subscribed = 0;
int finished = 0;

void subscriber_onDisconnect(void* context, MQTTAsync_successData* response) {
    printf("Successful disconnection\n");
    disc_finished = 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response) {
    printf("Subscribe succeeded\n");
    subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    printf("Subscribe failed, rc %d\n", response ? response->code : 0);
    finished = 1;
}

void subscriber_onConnect(void* context, MQTTAsync_successData* response) {
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc;
    printf("Successful connection\n");
    printf("Subscribing to topic %s using QoS%d\n\n"
       "Press Q<Enter> to quit\n\n", TOPIC, QOS);
    opts.onSuccess = onSubscribe;
    opts.onFailure = onSubscribeFailure;
    opts.context = client;
    deliveredtoken = 0;
    if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS) {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
}

void *subscriber_handler(void *targs) {
    thread_info *tinfo = targs;
    int id = tinfo->internal_id;
    int rc;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    int ch;

    char client_id[40];
    get_client_id(client_id, id);
    rc = MQTTAsync_create(&(tinfo->client), ADDRESS, client_id,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to create client, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    rc = MQTTAsync_setCallbacks(tinfo->client, NULL, connlost, msgarrvd, NULL);
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

    while   (!subscribed)
        usleep(10000L);

    if (finished)
        goto exit;

    do {
        ch = getchar();
    } while (ch!='Q' && ch != 'q');

    disc_opts.onSuccess = subscriber_onDisconnect;
    if ((rc = MQTTAsync_disconnect(tinfo->client, &disc_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start disconnect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    while (!disc_finished)
        usleep(10000L);
exit:
    MQTTAsync_destroy(&(tinfo->client));

    ///////

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    thread_info *tinfo;
    void *res;
    int rc;

    memset(connection_counter_per_thread, 0, sizeof(connection_counter_per_thread));
    memset(connection_finished, 0, sizeof(connection_finished));
    tinfo = malloc(sizeof(thread_info) * NUMBER_OF_CONCURRENT_THREADS);
    for (int i = 0; i < NUMBER_OF_CONCURRENT_THREADS; i++) {
        tinfo[i].internal_id = i;

        rc = pthread_create(&tinfo[i].thread, NULL,
                            subscriber_handler, (void*)&tinfo[i]);
        if (rc) {
            handle_error_en(rc, "pthread_create");
        }
    }
    sleep(1);

    for (int i = 0; i < NUMBER_OF_CONCURRENT_THREADS; i++) {
        rc = pthread_join(tinfo[i].thread, &res);

        if (rc) {
            handle_error_en(rc, "pthread_join");
        }

        free(res);
    }

    free(tinfo);
    exit(EXIT_SUCCESS);
}
