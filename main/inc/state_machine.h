#include <stdint.h>
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

typedef enum {
  STATE_SLEEP,
  STATE_ENTERING_PIN,
  STATE_DENIED,
  STATE_UNLOCKED,
  STATE_AUTHORIZED_OPENING,
  STATE_LOCKED_OUT,
  STATE_ALARM,
  STATE_RECOVERY
} LockState;

typedef enum {
  EVENT_NONE,
  EVENT_PIR_MOTION,
  EVENT_KEY_PRESSED,
  EVENT_PIN_CORRECT,
  EVENT_PIN_WRONG,
  EVENT_MAX_ATTEMPTS,
  EVENT_TIMEOUT,
  EVENT_INSIDE_HANDLE,
  EVENT_DOOR_FORCED_OPEN,
  EVENT_DOOR_AUTHORIZED_OPEN,
  EVENT_DOOR_CLOSED,
  EVENT_DEV_OVERRIDE
} LockEvent;

struct state_transition {
  LockState from_state;
  LockEvent event;
  LockState to_state;
};

void update_secret_hash(const uint8_t new_hash[32]);
void load_secret_hash(void);
void state_machine_init(void);
void state_machine_process_event(LockEvent event);
void state_machine_run(void);

#endif


