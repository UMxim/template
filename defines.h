/*
 * defines.h
 *
 *  Created on: Feb 23, 2026
 *      Author: Max
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#include "stdint.h"

#ifdef DEBUG
  #define ASSERT_DEBUG(err_stat) if(err_stat) { while(1);}
#else
  #define ASSERT_DEBUG(err_stat)
#endif

typedef enum
{
	ERROR_OK 							= 0,
	ERROR_WRONG_PARAM					= -1,
}errors_e;

typedef uint8_t bool;
#define false 	(0)
#define true 	(!false)

#define BIT_GET(reg, bit_num) 				(((reg) >> (bit_num)) & 1U)
#define BIT_GET_MASK(reg, mask) 			((reg) & (mask))
#define BIT_SET(reg, bit_num)				do {(reg) |= (1U << (bit_num)); } while(0)
#define BIT_CLEAR(reg, bit_num)				do {(reg) &= ~(1U << (bit_num)); } while(0)
#define BIT_MODIFY(reg, val, mask)			do {(reg) = ((reg) & ~(mask)) | ((val) & (mask)); } while(0)


#endif /* DEFINES_H_ */
