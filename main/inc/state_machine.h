#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

// 1. All the possible States
typedef enum {
  STATE_SLEEP,
  STATE_ENTERING_PIN,
  STATE_UNLOCKED,
  STATE_LOCKED_OUT,
  STATE_ALARM
} LockState;

// 2. All the possible Events (Things that can happen to the lock)
typedef enum {
  EVENT_NONE,
  EVENT_PIR_MOTION,      // Someone walked up to the door
  EVENT_KEY_PRESSED,     // Someone is typing on the keypad
  EVENT_PIN_CORRECT,     // The SHA-256 hash matched
  EVENT_PIN_WRONG,       // The SHA-256 hash failed
  EVENT_MAX_ATTEMPTS,    // They failed 5 times
  EVENT_TIMEOUT,         // They took too long, or the unlocked timer finished
  EVENT_INSIDE_HANDLE,   // Someone pressed the inside handle
  EVENT_DOOR_FORCED_OPEN,// Someone forced the door open without unlocking
  EVENT_DEV_OVERRIDE     // Developer mode button pressed!
} LockEvent;

// 3. The exact Transition Struct your tutor used!
struct state_transition {
  LockState from_state;
  LockEvent event;
  LockState to_state;
};

// 4. Function Prototypes
void state_machine_init(void);
void state_machine_process_event(LockEvent event);
void state_machine_run(void);

#endif


