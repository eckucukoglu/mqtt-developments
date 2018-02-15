# *****************************************************
# Parameters to control Makefile operation

# CC = gcc

LIBS = -lpaho-mqtt3as -lpthread

CFLAGS = -Wall
DEBUG = -g -DDEBUG

PUBLISHER = publisher
SUBSCRIBER = subscriber

# ****************************************************

all: publisher.o subscriber.o
	rm -f *.o

pub: publisher.o

sub: subscriber.o

publisher.o: MQTT_client.o MQTT_publish.c
	$(CC) $(CFLAGS) -o $(PUBLISHER) MQTT_publish.c MQTT_client.o $(LIBS)

subscriber.o: MQTT_client.o MQTT_subscribe.c
	$(CC) $(CFLAGS) -o $(SUBSCRIBER) MQTT_subscribe.c MQTT_client.o $(LIBS)

MQTT_client.o: MQTT_client.c MQTT_client.h
	$(CC) $(CFLAGS) -c MQTT_client.c $(LIBS)

debug: CFLAGS += $(DEBUG)
debug: all

clean:
	rm -f *.o
	rm -f $(PUBLISHER)
	rm -f $(SUBSCRIBER)
