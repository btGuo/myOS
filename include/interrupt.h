#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include "stdint.h"

typedef void* intr_handler;

enum intr_status{
	INTR_OFF,
	INTR_ON
};

enum intr_status intr_get_status(void);
enum intr_status intr_set_status(enum intr_status);
enum intr_status intr_enable(void);
enum intr_status intr_disable(void);
void idt_init();
void register_handler(uint8_t vec_nr, intr_handler function);

#endif
