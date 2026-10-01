#ifndef HANDLE_H
#define HANDLE_H
#include <stdbool.h>

void handle_init(void);
bool handle_is_pressed(void);
void handle_wait_for_release(void);

#endif
