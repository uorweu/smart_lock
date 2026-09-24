#ifndef KEYPAD_H
#define KEYPAD_H
#include <stdint.h>
void keypad_init(void);
uint8_t keypad_get_key(void);
#endif 
