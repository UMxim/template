/*
 * template.h
 *
 *  Created on: Feb 15, 2026
 *      Author: Max
 */

#ifndef TEMPLATE_H_
#define TEMPLATE_H_

#include "template_config.h"
#include "defines.h"
#include "pt.h"
#include "max_hal.h"
#include "param.h"

// ----------------------------------------------------------------- timer -----------------------------------------------------------------

extern volatile uint64_t systime_ticks;

#define US_TO_TICKS(us) ((uint64_t)(us) * TEMPLATE_SYSTICK_FREQ / 1000000ULL)

uint64_t timer_set_timestamp_ticks(uint64_t duration_ticks);
#define  timer_set_timestamp_us(duration_us)			timer_set_timestamp_ticks(US_TO_TICKS(duration_us))

bool timer_is_timer_expired(uint64_t timestamp);

void	 timer_delay_ticks(uint64_t delay_ticks);
#define  timer_delay_us(us) 							timer_delay_ticks(US_TO_TICKS(us))
#define  timer_delay_ms(ms) 							timer_delay_ticks(1000 * US_TO_TICKS(us))

// ----------------------------------------------------------------- misc -------------------------------------------------------------------

uint32_t xor32(uint32_t *data, uint32_t size_word);

// ----------------------------------------------------------------- template ---------------------------------------------------------------

void template_init(void);

PT_THREAD template_cycle(void);

#endif /* TEMPLATE_H_ */
