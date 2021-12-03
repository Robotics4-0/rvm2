#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include <assert.h>
#include <MQTTClient.h>
#include "mqtt_handler.h"



#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "sim"
#define TOPIC       "rvm2/coordinate"
#define QOS         0
#define TIMEOUT     10000L

#define MIN_DELTA_T 100 //milliseconds

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;
int rc;

char incoming_message[50] = "";
char incoming_flag = 0;

//forward declaration
void publish_cordinate(coord_t);
int subscribe_callback(void* , char*, int, MQTTClient_message*);

bool coord_equal(coord_t c1, coord_t c2){
    
    if( (int)(c1.x*10)==(int)(c2.x*10) &&
        (int)(c1.y*10)==(int)(c2.y*10) &&
        (int)(c1.z*10)==(int)(c2.z*10) &&
        (int)(c1.pitch*10)==(int)(c2.pitch*10) &&
        (int)(c1.roll*10)==(int)(c2.roll*10) &&
        c1.claw==c2.claw
    )
        return True;
    return False;
}

bool coord_is_valid(coord_t c){
    
    if( c.x > 1 && c.x < 1 &&
        c.y > 1 && c.y < 1 &&
        c.z > 1 && c.z < 1 &&
        c.pitch > 1 && c.pitch < 1 &&
        c.roll > 1 && c.roll < 1
    )
        return True;
    return False;
}

/**
 * @brief Initialize the handler
 */
void mqtt_handler_init(){

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
  
    // MQTT Connection parameters
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    rc = MQTTClient_setCallbacks(client, NULL, NULL, subscribe_callback, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }

    if (rc != MQTTCLIENT_SUCCESS)
	{
		MQTTClient_destroy(&client);
		return;
	}

    rc = MQTTClient_subscribe(client, TOPIC, QOS);
	//assert("Good rc from subscribe", rc == MQTTCLIENT_SUCCESS, "rc was %d", rc);
}

/**
 * @brief Close the handler
 */
void mqtt_handler_close(){

    // Disconnect
    rc = MQTTClient_unsubscribe(client, TOPIC);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

/**
 * @brief this function must be called on the infinite loop
 * @param c pointer to actual coordintes
 */
void mqtt_periodic_callback(coord_t* coord){

    static coord_t last_coord;
    static time_t last_time;
    time_t now = clock();

    //check if min delta t has passed
    if ( (int)(now-last_time)/(CLOCKS_PER_SEC*1000) >= MIN_DELTA_T){
    
        last_time = now;
        
        if ( !coord_equal(last_coord,*coord) ){
            last_coord = *coord;
            publish_cordinate(last_coord);
        }
    }

    //check subscription
    //take mutex
    if (incoming_flag){
        incoming_flag = 0;

        coord_t aux;
        char claw = 0;
        sscanf(incoming_message, "%+.1f, %+.1f, %+.1f, %+.1f, %+.1f, %c",
                &aux.x, &aux.y, &aux.z, &aux.pitch, &aux.roll, &claw);
        aux.claw = (claw=='C') ? 0 : 1;

        if ( coord_is_valid(aux) ){
            *coord = aux;   
        }
    }
    //release mutex

    
}

void publish_cordinate(coord_t c){
    char message[50];
    sprintf(&message,
            "%+.1f, %+.1f, %+.1f, %+.1f, %+.1f, %c",
            c.x, c.y, c.z, c.pitch, c.roll, c.claw?'O':'C');

    // Publish message
    pubmsg.payload = message;
    pubmsg.payloadlen = strlen(message);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);

}

int subscribe_callback(void* context, char* topicName, int topicLen, MQTTClient_message* m)
{
    if ( m->payloadlen >= sizeof(incoming_message) )
        return 1; //reject if message is too big
    
    //take mutex
	strncpy(incoming_message, m->payload, m->payloadlen);
    incoming_message[m->payloadlen] = '\0'; //terminate string
    incoming_flag = 1;
    //release mutex

	MQTTClient_free(topicName);
	MQTTClient_freeMessage(&m);
	return 1;
}


