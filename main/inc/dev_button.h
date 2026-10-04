#ifndef DEV_BUTTON_H
#define DEV_BUTTON_H

#include <stdbool.h>

void dev_button_init(void);

extern volatile bool dev_override_flag;

#endif
