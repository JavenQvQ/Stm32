#ifndef __OS_DELAY_H
#define __OS_DELAY_H

#include "stdint.h"

void delay_init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif
