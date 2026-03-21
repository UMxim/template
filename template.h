/*
 * template.h
 *
 *  Created on: Feb 15, 2026
 *      Author: Max
 */

#ifndef TEMPLATE_H_
#define TEMPLATE_H_

#include "defines.h"
#include "pt.h"
#include "max_hal.h"
#include "param.h"

// ----------------------------------------------------------------- timer -----------------------------------------------------------------
#define TIMER_TICK_PER_US	32

extern volatile uint64_t systime_ms;

uint64_t timer_set_timestamp(uint64_t duration);

bool 	timer_is_timer_expired(uint64_t timestamp);

void 	timer_delay_ms(uint32_t delay); // Блокирующая ф-ия. uint32_t - неужели надо зависнуть более 49 дней?

void 	timer_delay_us(uint32_t delay); // В пределах 1мс

// ----------------------------------------------------------------- template ---------------------------------------------------------------

void template_init(void);

PT_THREAD template_cycle(void);

#endif /* TEMPLATE_H_ */
