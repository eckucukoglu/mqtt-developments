#include "MQTT_client.h"

#define CLIENTID    "ExampleClientSub"
#define PAYLOAD     "Hello World!"

int disc_finished = 0;
int subscribed = 0;
int finished = 0;

void onDisconnect(void* context, MQTTAsync_successData* response) {
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

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("Connect failed, rc %d\n", response ? response->code : 0);
    finished = 1;
}

void onConnect(void* context, MQTTAsync_successData* response) {
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    int rc;
    printf("Successful connection\n");
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
       "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    opts.onSuccess = onSubscribe;
    opts.onFailure = onSubscribeFailure;
    opts.context = client;
    deliveredtoken = 0;
    if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS) {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_token token;
    int rc;
    int ch;
    MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    rc = MQTTAsync_setCallbacks(client, NULL, connlost, msgarrvd, NULL);
    printf("%d\n", rc);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;

    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
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

    disc_opts.onSuccess = onDisconnect;
    if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS) {
            printf("Failed to start disconnect, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }

    while   (!disc_finished)
        usleep(10000L);
exit:
    MQTTAsync_destroy(&client);
    exit(EXIT_SUCCESS);
}
