#include "template.h"


volatile uint64_t systime_ms = 0;

// ===== timer =====

static inline uint64_t timer_get_time()
{
	uint64_t time;
	do
	{
		time = systime_ms;
	}while(time != systime_ms);
	return time;
}

inline uint64_t timer_set_timestamp(uint64_t duration)
{
	return timer_get_time() + duration;
}

inline bool timer_is_timer_expired(uint64_t timestamp)
{
	return ( timer_get_time() >= timestamp );
}

void 	timer_delay_ms(uint32_t delay)
{
	uint64_t timestamp = timer_set_timestamp(delay);
	while(!timer_is_timer_expired(timestamp)) ;
}

void timer_delay_us(uint32_t delay_in)
{
    uint32_t total_ticks = delay_in * TIMER_TICK_PER_US;

    uint32_t passed = 0;
    uint32_t last = SysTick->VAL;
    uint32_t current;

    while (passed < total_ticks)
    {
        current = SysTick->VAL;
        passed += (last - current) & 0xFFFFFF;
        last = current;
    }
}

// === ===
#ifdef STM32L011xx

#endif

void template_init(void)
{
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk; // почему-то не было
	OB_disable_boot0_pin();// разберись с boot0 на stm32l011 cubeProgrammer
	__enable_irq();
	params_init();

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
