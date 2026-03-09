/*
 * pt.h
 *
 *  Created on: Feb 24, 2026
 *      Author: Max
 */

#ifndef PT_H_
#define PT_H_

#include "stdint.h"

typedef enum
{
	PT_WAITING		= 0,
	PT_EXITED  		= 1,
	PT_ENDED   		= 2,
	PT_YIELDED 		= 3,
}PT_THREAD;

typedef uint16_t pt_t;

// === Private ===
#define PT_LC_INIT(s) s = 0;

#define PT_LC_RESUME(s) switch(s) { case 0:

#define PT_LC_SET(s) s = __LINE__; case __LINE__:

#define PT_LC_END(s) }

// === Public ===

#define PT_INIT(pt)   	PT_LC_INIT(pt)

#define PT_BEGIN(pt) 	{ char PT_YIELD_FLAG = 1; PT_LC_RESUME(pt)

#define PT_END(pt) 		PT_LC_END(pt); PT_YIELD_FLAG = 0; 				\
                   	   	PT_INIT(pt); return PT_ENDED; }

#define PT_WAIT_UNTIL(pt, condition)	        						\
						do {											\
							PT_LC_SET(pt);								\
							if(!(condition)) {	return PT_WAITING;	}	\
							} while(0)

#define PT_WAIT_WHILE(pt, cond)  PT_WAIT_UNTIL((pt), !(cond))

#define PT_RESTART(pt)													\
						do {											\
							PT_INIT(pt);								\
							return PT_WAITING;							\
						} while(0)

#define PT_EXIT(pt)														\
						do {											\
							PT_INIT(pt);								\
							return PT_EXITED;							\
						} while(0)

#define PT_YIELD(pt)													\
						do {											\
							PT_YIELD_FLAG = 0;							\
							PT_LC_SET(pt);								\
							if(PT_YIELD_FLAG == 0) return PT_YIELDED;	\
  	  	  	  	  	  	} while(0)

#define PT_DELAY_MS(pt, timestamp, duration)													\
						do {	template_timer_set_timestamp(&timestamp, duration);				\
								PT_WAIT_UNTIL(pt, template_timer_is_timer_expired(&timestamp));	\
						} while(0)

#endif /* PT_H_ */
