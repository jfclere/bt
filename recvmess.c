#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <zlib.h>
#include "MQTTAsync.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientSub"
/* "topic/#" so we receive topic/dongle1, dongle2 etc */
#define TOPIC       "topic/#"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L
#define USERNAME    "admin"
#define PASSWORD    "admin"

int disc_finished = 0;
int subscribed = 0;
int finished = 0;

void inserttemp(char *table, time_t t, float temp, float pres, float humi);

void onConnect(void* context, MQTTAsync_successData* response);
void onConnectFailure(void* context, MQTTAsync_failureData* response);

void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
        conn_opts.username = USERNAME;
        conn_opts.password = PASSWORD;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		finished = 1;
	}
}


int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    time_t time;
    float temp;
    float pres;
    float humi;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);
    /* topic/dongle(n), use dongle(n) as table */
    char *devicename = strrchr(topicName, '/');
    devicename++;
    // sscanf(message->payload, "%d %4.2f %6.2f %4.2f", &time, &temp, &pres, &humi);
    sscanf(message->payload, "%d %f %f %f", &time, &temp, &pres, &humi);
    printf("%d %4.2f %6.2f %4.2f\n", time, temp, pres, humi);
    inserttemp(devicename, time, temp, pres, humi);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Disconnect failed, rc %d\n", response->code);
	disc_finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	disc_finished = 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
	subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response->code);
	finished = 1;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response->code);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	printf("Successful connection\n");

	printf("Subscribing to topic %s\nfor client %s using QoS%d\n",
           TOPIC, CLIENTID, QOS);
	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;
	if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		finished = 1;
	}
}

int main(int argc, char* argv[])
{
	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	int rc;
	int ch;

	if ((rc = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL))
			!= MQTTASYNC_SUCCESS)
	{
		printf("Failed to create client, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto exit;
	}

	if ((rc = MQTTAsync_setCallbacks(client, client, connlost, msgarrvd, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to set callbacks, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
        conn_opts.username = USERNAME;
        conn_opts.password = PASSWORD;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	while (!finished) {
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif
        }

        printf("finished: %d subscribed %d\n", finished, subscribed);
        fflush(stdout);
	if (finished)
		goto exit;

	disc_opts.onSuccess = onDisconnect;
	disc_opts.onFailure = onDisconnectFailure;
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}
 	while (!disc_finished)
 	{
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif
 	}

destroy_exit:
	MQTTAsync_destroy(&client);
exit:
 	return rc;
}
