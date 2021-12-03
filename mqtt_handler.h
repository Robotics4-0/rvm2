#ifndef _MQTT_HANDLER_H_
#define _MQTT_HANDLER_H_

#ifndef __cplusplus
typedef unsigned char bool;
static const bool False = 0;
static const bool True = 1;
#endif

typedef struct {
    float x,y,z,pitch,roll;
    bool claw;
} coord_t;

void mqtt_handler_init();
void mqtt_handler_close();
void mqtt_periodic_callback(coord_t*);



#endif //_MQTT_HANDLER_H_

