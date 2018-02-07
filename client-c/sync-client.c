#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <unistd.h>
#include "MQTTClient.h"

#define ADDRESS     "tcp://10.106.231.44:1883"
#define CLIENTID    "c-"
#define TOPIC       "sync-client"
#define PAYLOAD     "Hello World"
#define QOS         1
#define TIMEOUT     10000L

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printf("arg needed: number of clients\n");
        return 1;
    }

    int count = 0;
    int number_of_clients = atoi(argv[1]);
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    int rc;

    for (int i = 0; i < number_of_clients; i++) {
        char scount[6], client_id[10];
        sprintf(scount, "%d", i);
        memcpy(client_id, CLIENTID, 2);
        memcpy(client_id+2, scount, strlen(scount));

        MQTTClient_create(&client, ADDRESS, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);

        if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
            printf("Failed to connect, return code %d\n", rc);
        } else {
            count++;
        }

        printf("Count: %d\n", count);

        // MQTTClient_destroy(&client);
    }

    printf("Count: %d\n", count);
    sleep(20);

    // MQTTClient_deliveryToken token;
    // MQTTClient_message pubmsg = MQTTClient_message_initializer;
    // pubmsg.payload = PAYLOAD;
    // pubmsg.payloadlen = strlen(PAYLOAD);
    // pubmsg.qos = QOS;
    // pubmsg.retained = 0;
    // MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    // printf("Waiting for up to %d seconds for publication of %s\n"
    //         "on topic %s for client with ClientID: %s\n",
    //         (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
    // rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    // printf("Message with delivery token %d delivered\n", token);
    // MQTTClient_disconnect(client, 10000);
    // MQTTClient_destroy(&client);

    return 0;
}
