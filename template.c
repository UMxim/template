#include "template.h"

// ----------------------------------------------------------------- timer -----------------------------------------------------------------

volatile uint64_t systime_ticks = 0;

static uint64_t timer_get_ticks()
{
	uint64_t time;
	uint32_t fract;
	do
	{
		time = systime_ticks;
		fract = SysTick->LOAD - SysTick->VAL;
	}while(time != systime_ticks);
	return time + fract;
}

uint64_t timer_set_timestamp_ticks(uint64_t duration_ticks)
{
	return timer_get_ticks() + duration_ticks;
}

bool timer_is_timer_expired(uint64_t timestamp)
{
	return ( timer_get_ticks() >= timestamp );
}

void timer_delay_ticks(uint64_t delay_ticks)
{
	uint64_t timestamp = timer_set_timestamp_ticks(delay_ticks);
	while(!timer_is_timer_expired(timestamp)) ;
}

// ----------------------------------------------------------------- misc -------------------------------------------------------------------

uint32_t xor32(uint32_t *data, uint32_t size_word)
{
	uint32_t xor = 0;
	while(size_word--)
	{
		xor ^= *data++;
	}
	return xor;
}

// ----------------------------------------------------------------- template ---------------------------------------------------------------

void template_init(void)
{
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk; // почему-то не было
	//OB_disable_boot0_pin();// разберись с boot0 на stm32l011 cubeProgrammer
	__enable_irq();
	//params_init();

}

PT_THREAD template_cycle(void)
{
	static pt_t pt = 0;
	PT_BEGIN(pt);
	while(1)
	{
		params_cycle();
		PT_YIELD(pt);
	}
	PT_END(pt);
}
