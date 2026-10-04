#ifndef MQTT_H
#define MQTT_H

typedef enum {
  LOCK_STATE_LOCKED = 0,
  LOCK_STATE_UNLOCKED = 1,
  LOCK_STATE_JAMMED = 2
}lock_state_t;

void mqtt_init(void);
void mqtt_start(void);
#include "state_machine.h"
void mqtt_publish_state(LockState state);
#endif
