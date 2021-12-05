#ifndef _MQTT_HANDLER_H_
#define _MQTT_HANDLER_H_


typedef struct {
    float x,y,z,pitch,roll;
    bool claw;
} coord_t;

void mqtt_handler_init();
void mqtt_handler_close();
int  mqtt_periodic_callback(coord_t*);



#endif //_MQTT_HANDLER_H_

