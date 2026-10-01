#ifndef DEV_BUTTON_H
#define DEV_BUTTON_H

#include <stdbool.h>

void dev_button_init(void);

// This flag is set to true when the interrupt fires!
extern volatile bool dev_override_flag;

#endif
