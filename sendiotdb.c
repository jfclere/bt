
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include "MQTTAsync.h"
/* ADDRESS should a parameter */
#define ADDRESS     "tcp://192.168.1.160:1883"
/* #define ADDRESS     "mqtts://jfclere.myddns.me:8001" */
#define CLIENTID    "ExampleClientPub"
/* we use topic/# in receiver so /topic/dongle(n) here */
#define TOPIC       "topic/root.bme280"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L
#define USERNAME    "root"
#define PASSWORD    "root"

struct info {
   float temp;
   float pres;
   float humi;
};
char *filename;

/**
* @brief provide same output with the native function in java called
* currentTimeMillis().
*/
int64_t currentTimeMillis() {
  struct timeval time;
  gettimeofday(&time, NULL);
  int64_t s1 = (int64_t)(time.tv_sec) * 1000;
  int64_t s2 = (time.tv_usec / 1000);
  return s1 + s2;
}

void getinfo(struct info *info, char *filename)
{
    char mess[100] = { 0 };
    int time;
    float temp, pres, humi;
    int fd = open(filename, O_RDONLY, 0);
    read(fd, mess, sizeof(mess));
    close(fd);
    printf("getinfo: %s from %s\n", mess, filename);
    sscanf(mess, "%d %f %f %f", &time, &temp, &pres, &humi); 
    info->temp = temp;
    info->pres = pres;
    info->humi = humi;
}

volatile MQTTAsync_token deliveredtoken;
int finished = 0;
void connlost(void *context, char *cause)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        int rc;
        printf("\nConnection lost\n");
        printf("     cause: %s\n", cause);
        printf("Reconnecting\n");
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start connect, return code %d\n", rc);
                finished = -1;
        }
}
void onDisconnect(void* context, MQTTAsync_successData* response)
{
        printf("Successful disconnection\n");
        finished = 1;
}
void onSend(void* context, MQTTAsync_successData* response)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
        int rc;
        printf("Message with token value %d delivery confirmed\n", response->token);
        opts.onSuccess = onDisconnect;
        opts.context = client;
        if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start sendMessage, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
}
void onFailure(void* context, MQTTAsync_failureData *response)
{
        printf("Message with token value %d failed\n", response->token);
        if (response->message)
                printf("Code %d, %s\n", response->code, response->message);
        else
                printf("Code %d\n");
        finished = -1;
}
void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
        printf("Connect failed, rc %d\n", response ? response->code : 0);
        finished = -1;
}
/* Note the device root.bme280, created as:
 * CREATE DATABASE root;
 * create device template t1 (temperature FLOAT encoding=RLE, pression FLOAT encoding=RLE, humidy FLOAT encoding=RLE, bat FLOAT encoding=RLE)
 * set device template t1 to root.bme280;
 * create timeseries using device template on root.bme280;
 */
void onConnect(void* context, MQTTAsync_successData* response)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        int rc;
        char mess[1024];
        struct info info;
        printf("Successful connection\n");
        opts.onSuccess = onSend;
        opts.onFailure = onFailure;
        opts.context = client;
        /* table created as:
         *   CREATE TABLE measurements (
         *  time bigint,
         *  temp NUMERIC(4,2),
         *  pres NUMERIC(6,2),
         *  humi NUMERIC(5,2));
         */
        getinfo(&info, filename);

        int64_t t = currentTimeMillis();

        char *devicenametxt = strrchr(filename, '/');
        devicenametxt++;
        char *devicename = strdup(devicenametxt);
        char *dot = strchr(devicename, '.');
        *dot = '\0';
        char device[100];
        strcpy(device, "root.");
        strcat(device, devicename);

        /* build the json mess  */
        sprintf(mess, "{\n \"device\":\"%s\",\n \"timestamp\":%lld,\n \"measurements\":[\"temperature\",\"pression\",\"humidy\",\"bat\"],\n \"values\":[%4.2f,%6.2f,%4.2f,%4.2f]\n}", device, t, info.temp, info.pres, info.humi, 0.0);
        pubmsg.payload = mess;
        pubmsg.payloadlen = strlen(mess);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        deliveredtoken = 0;

        char topic[100];
        sprintf(topic,"topic/%s", devicename);
        printf("Sending %s on %s\n", mess, topic);

        if ((rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start sendMessage, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
}
int main(int argc, char* argv[])
{
        MQTTAsync client;
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        MQTTAsync_token token;
        MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
        int rc;

        if (argc != 2) {
            printf("Need file name\n");
            exit(1);
        }
        filename = argv[1];
        int fd = open(filename, O_RDONLY, 0);
        if (fd < 0) {
            printf("Can't open %s\n", filename);
            exit(1);
        }
        close(fd);

        rc = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
        if (rc != MQTTASYNC_SUCCESS) {
            printf("Failed to create client object, return code %d\n", rc);
            exit(1);
        }
        MQTTAsync_setCallbacks(client, NULL, connlost, NULL, NULL);
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        conn_opts.onSuccess = onConnect;
        conn_opts.onFailure = onConnectFailure;
        conn_opts.context = client;
        conn_opts.username = USERNAME;
        conn_opts.password = PASSWORD;
        ssl_opts.enableServerCertAuth = false;
        ssl_opts.verify = false;
/* XXX something more ...
    ssl_opts.struct_version = 3;
    ssl_opts.trustStore = server_certificate.c_str();
    ssl_opts.keyStore = client_certificate.c_str();
    ssl_opts.privateKey = private_key.c_str();
    ssl_opts.privateKeyPassword = private_key_password.c_str();
 */

        conn_opts.ssl = &ssl_opts;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start connect, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
        printf("Waiting for publication of %s\n"
         "on topic %s for client with ClientID: %s\n",
         PAYLOAD, TOPIC, CLIENTID);
        int loop = 0;
        while (!finished) {
                #if defined(WIN32) || defined(WIN64)
                        Sleep(100);
                #else
                        usleep(10000L);
                #endif
                loop++;
                if (loop == 6000) {
                        finished = -1; /* timeout after 60 seconds */ 
                }
        }
        MQTTAsync_destroy(&client);
        if (finished == -1) {
            printf("Something failed\n");
            exit(1);
        }
        return rc;
}
