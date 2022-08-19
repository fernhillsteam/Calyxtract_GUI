
#include <stdint.h>
#include <stdbool.h>

#define TARGET_IS_TM4C129_RA1

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_timer.h"
#include "inc/hw_udma.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/udma.h"
#include "dbg.h"
#include "encoder.h"

static uint32_t ui32EncPulses;
static uint32_t pulses;

void enc_init(void)
{

}

void enc_latch_count(void)
{
	pulses = TimerValueGet(TIMER0_BASE, TIMER_A);
}

uint32_t enc_get_latched_count(void)
{
	return pulses;
}


void enc_flag_detection(void)
{

}

void enc_count_handler(void)
{
	GPIOIntClear(GPIO_PORTD_BASE, GPIO_INT_PIN_0);

	ui32EncPulses++;
}
