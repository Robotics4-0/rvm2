#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "stdbool.h"
#include <assert.h>
#include <pthread.h>

#include <MQTTClient.h>
#include "mqtt_handler.h"


MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;
int rc;

char incoming_message[50] = "";
char incoming_flag = 0;

pthread_mutex_t incoming_mutex;

//forward declaration
void publish_cordinate(coord_t);
int subscribe_callback(void* , char*, int, MQTTClient_message*);
/*
bool coord_equal(coord_t c1, coord_t c2){
    
    if( (int)(c1.x*10)==(int)(c2.x*10) &&
        (int)(c1.y*10)==(int)(c2.y*10) &&
        (int)(c1.z*10)==(int)(c2.z*10) &&
        (int)(c1.pitch*10)==(int)(c2.pitch*10) &&
        (int)(c1.roll*10)==(int)(c2.roll*10) &&
        c1.claw==c2.claw
    )
        return true;
    return false;
}*/

bool coord_equal(coord_t c1, coord_t c2, float epsilon){
    if( fabs( c1.x-c2.x ) < epsilon &&
        fabs( c1.y-c2.y ) < epsilon &&
        fabs( c1.z-c2.z ) < epsilon &&
        fabs( c1.pitch-c2.pitch ) < epsilon &&
        fabs( c1.roll-c2.roll ) < epsilon &&
        c1.claw==c2.claw
    )
        return true;
    return false;
}

bool coord_is_valid(coord_t c){
    return true; //TODO: update values
    if( c.x > 1 && c.x < 1 &&
        c.y > 1 && c.y < 1 &&
        c.z > 1 && c.z < 1 &&
        c.pitch > 1 && c.pitch < 1 &&
        c.roll > 1 && c.roll < 1
    )
        return true;
    return false;
}

/**
 * @brief Initialize the handler
 */
void mqtt_handler_init(){

    pthread_mutex_init(&incoming_mutex, NULL);

    char clientID[10];
    //generate unique ID
    sprintf(&clientID, "sim%d", time(0)%100000);

    MQTTClient_create(&client, ADDRESS, clientID,
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
int mqtt_periodic_callback(coord_t* coord){
    static int run_flag=0;
    static coord_t last_coord;
    static time_t last_time = 0;
    time_t now = clock();
    int ret_flag = 0;

    //check if min delta t has passed
    if ( ((double)(now - last_time) / (double)CLOCKS_PER_SEC) >= (double)MIN_DELTA_T){
    
        last_time = now;
        
        //if this is the first time
        if (run_flag == 0){ 
            run_flag = 1;
            last_coord = *coord; //this will prevent publish
        }

        //check if simulator has moved
        if ( !coord_equal(last_coord,*coord, EPSILON) ){
            last_coord = *coord; //publish movement
            publish_cordinate(last_coord);
        }
    }

    //check subscription
    //take mutex
    pthread_mutex_lock(&incoming_mutex);

    if (incoming_flag){ //if a message arrived
        incoming_flag = 0;

        coord_t aux;
        char claw = 0;
        //decode the message
        sscanf(incoming_message, "%f, %f, %f, %f, %f, %c",
                &aux.x, &aux.y, &aux.z, &aux.pitch, &aux.roll, &claw);
        aux.claw = (claw=='C') ? 0 : 1;

        //check if message is valid
        if ( coord_is_valid(aux) ){
            //only apply if movement is significant
            if (!coord_equal(last_coord, aux, EPSILON)){
                printf("\nAPPLY");
                *coord = aux; //apply coordinates
                last_coord = aux; //remember coordinates
                ret_flag = 1; //notify update         

            }
        }
    }

    //release mutex
    pthread_mutex_unlock(&incoming_mutex);

    return ret_flag;

    
}

void publish_cordinate(coord_t c){
    char message[50];
    sprintf(&message,
            "%+.1f, %+.1f, %+.1f, %+.1f, %+.1f, %c",
            c.x, c.y, c.z, c.pitch, c.roll, c.claw?'O':'C');

    printf("\npublish:  %s", message);
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
    pthread_mutex_lock(&incoming_mutex);

	strncpy(incoming_message, m->payload, m->payloadlen);
    incoming_message[m->payloadlen] = '\0'; //terminate string
    incoming_flag = 1;

    printf("\nsubscribe:%s", incoming_message);
    pthread_mutex_unlock(&incoming_mutex);
    //release mutex

	MQTTClient_free(topicName);
	MQTTClient_freeMessage(&m);
	return 1;
}


