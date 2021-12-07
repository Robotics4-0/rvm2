#ifndef _MQTT_HANDLER_H_
#define _MQTT_HANDLER_H_


#define ADDRESS     "tcp://localhost:1883"
#define TOPIC       "rvm2/coordinate"
#define QOS         0
#define TIMEOUT     10000L

#define MIN_DELTA_T 0.1 //seconds
#define EPSILON 0.1   //min error


typedef struct {
    float x,y,z,pitch,roll;
    bool claw;
} coord_t;

void mqtt_handler_init();
void mqtt_handler_close();
int  mqtt_periodic_callback(coord_t*);
bool coord_equal(coord_t, coord_t, float);



#endif //_MQTT_HANDLER_H_

