
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
#include "motor.h"

#define NUM_ELEMENTS(x)          (sizeof(x)/sizeof(x[0]))


#define FULL_ROTATION            (0)
#define HALF_ROTATION            (1)
#define MOTOR_LOGIC              (HALF_ROTATION)

#define MICROSTEP                (32)
#define FULL_ROTATION_COUNT      (200)
#define HALF_ROTATION_COUNT      (FULL_ROTATION_COUNT/2)

//static const uint32_t pwm_load_values[5] = { 75000, 12500, 5000, 5000, 5000 };
//static const uint32_t pwm_load_values[5] = { 25000, 9375, 7500, 5625, 7500 };
//static const uint32_t pwm_load_values[5] = { 37500, 9375, 7500, 5625, 9375 };
//static const uint32_t pwm_load_values[5] = { 75000, 9375, 7500, 5625, 9375 };
#if MOTOR_LOGIC == FULL_ROTATION
static uint32_t pwm_load_values[10] = { 62500, 9375, 7500, 7500, 5625, 5625, 5625, 5625, 7500, 7500 };
#elif MOTOR_LOGIC == HALF_ROTATION
static uint32_t *pwm_load_values;
//static uint32_t pwm_load_values_forward[5] = { 62500, 9375, 7500, 7500, 5625};
static uint32_t pwm_load_values_forward[5] = { 62500, 18500, 9375, 9375, 9375};
static uint32_t pwm_load_values_backward[5] = { 62500, 18500, 9375, 9375, 9375};
static uint_fast8_t half_rotation;
#endif
static uint_fast8_t pwm_load_idx = 0;
static volatile uint_fast8_t homing = 0;
static volatile uint_fast8_t re_start = 0;
volatile uint8_t busy = 0;

void motor_init(void)
{

}

void motor_start(void)
{
	uint8_t pr;
	uint16_t ilr;

	dbg_printf("Motor Start\r\n");

	busy = 1;

	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerIntClear(TIMER1_BASE, TIMER_CAPA_MATCH);
	GPIOIntClear(GPIO_PORTL_BASE, GPIO_INT_PIN_0);
	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMB_TIMEOUT);

	ROM_GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_1, 0);

#if MOTOR_LOGIC == FULL_ROTATION
	TimerLoadSet(TIMER1_BASE, TIMER_A, 0xFFFF);
	TimerMatchSet(TIMER1_BASE, TIMER_A, (FULL_ROTATION_COUNT*MICROSTEP)-1);

	pwm_load_idx = 0;
	pr = pwm_load_values[pwm_load_idx] >> 16;
	TimerPrescaleSet(TIMER1_BASE, TIMER_B, pr);
	ilr = pwm_load_values[pwm_load_idx] & 0xFFFFU;
	TimerLoadSet(TIMER1_BASE, TIMER_B, ilr);

	pr = pwm_load_values[pwm_load_idx] >> 17;
	TimerPrescaleMatchSet(TIMER1_BASE, TIMER_B, pr);
	ilr = (pwm_load_values[pwm_load_idx] >> 1) & 0xFFFFU;
	TimerMatchSet(TIMER1_BASE, TIMER_B, ilr);

	MAP_uDMAChannelTransferSet(UDMA_CH6_TIMER2A|UDMA_PRI_SELECT,
							UDMA_MODE_BASIC,
							&pwm_load_values[1],
							&HWREG(TIMER1_BASE + TIMER_O_TBILR),
							9);

	//ROM_uDMAChannelEnable(UDMA_CH6_TIMER2A);

#elif MOTOR_LOGIC == HALF_ROTATION
	half_rotation = 1;
	TimerLoadSet(TIMER1_BASE, TIMER_A, 0xFFFF);
	TimerMatchSet(TIMER1_BASE, TIMER_A, (HALF_ROTATION_COUNT*MICROSTEP)-1);

	pwm_load_idx = 0;
	pwm_load_values = &pwm_load_values_forward[0];
	pr = pwm_load_values[pwm_load_idx] >> 16;
	TimerPrescaleSet(TIMER1_BASE, TIMER_B, pr);
	ilr = pwm_load_values[pwm_load_idx] & 0xFFFFU;
	TimerLoadSet(TIMER1_BASE, TIMER_B, ilr);

	pr = pwm_load_values[pwm_load_idx] >> 17;
	TimerPrescaleMatchSet(TIMER1_BASE, TIMER_B, pr);
	ilr = (pwm_load_values[pwm_load_idx] >> 1) & 0xFFFFU;
	TimerMatchSet(TIMER1_BASE, TIMER_B, ilr);

	homing = 0;
#endif
	// Start edge count
	TimerEnable(TIMER1_BASE, TIMER_A);
	// Start PWM
	TimerEnable(TIMER1_BASE, TIMER_B);
	// Start timer to handle PWM ramp
	TimerEnable(TIMER2_BASE, TIMER_A);
}

void motor_ramp_timer_handler(void)
{
	uint8_t pr;
	uint16_t ilr;

	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);

	++pwm_load_idx;
#if MOTOR_LOGIC == FULL_ROTATION
	if (pwm_load_idx >= NUM_ELEMENTS(pwm_load_values))
#elif MOTOR_LOGIC == HALF_ROTATION
	if (pwm_load_idx >= NUM_ELEMENTS(pwm_load_values_forward))
#endif
	{
		// Stop Ramp Timer
		ROM_TimerDisable(TIMER2_BASE, TIMER_A);
		ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	}
	else
	{
		pr = pwm_load_values[pwm_load_idx] >> 16;
		TimerPrescaleSet(TIMER1_BASE, TIMER_B, pr);
		ilr = pwm_load_values[pwm_load_idx] & 0xFFFFU;
		TimerLoadSet(TIMER1_BASE, TIMER_B, ilr);

		pr = pwm_load_values[pwm_load_idx] >> 17;
		TimerPrescaleMatchSet(TIMER1_BASE, TIMER_B, pr);
		ilr = (pwm_load_values[pwm_load_idx] >> 1) & 0xFFFFU;
		TimerMatchSet(TIMER1_BASE, TIMER_B, ilr);
	}
}

void motor_pulse_count_handler(void)
{
	ROM_TimerIntClear(TIMER1_BASE, TIMER_CAPA_MATCH);
	ROM_TimerIntClear(TIMER1_BASE, TIMER_CAPA_MATCH);

	// Stop Pulse count Timer
	TimerLoadSet(TIMER1_BASE, TIMER_A, 0xFFFF);
	ROM_TimerDisable(TIMER1_BASE, TIMER_A);

	// Stop PWM Timer
	ROM_TimerDisable(TIMER1_BASE, TIMER_B);

	// Stop Ramp Timer
	ROM_TimerDisable(TIMER2_BASE, TIMER_A);

	// Disable homing interrupt
	GPIOIntDisable(GPIO_PORTL_BASE, GPIO_INT_PIN_0);

	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerIntClear(TIMER1_BASE, TIMER_CAPA_MATCH);
	GPIOIntClear(GPIO_PORTL_BASE, GPIO_INT_PIN_0);
	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMB_TIMEOUT);

	homing = 0;

#if MOTOR_LOGIC == HALF_ROTATION

	if (half_rotation)
	{
		half_rotation = 0;
		TimerLoadSet(TIMER2_BASE, TIMER_B, 25000-1);
		// Start timer to wait
		TimerEnable(TIMER2_BASE, TIMER_B);

		ROM_GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_1, GPIO_PIN_1);

	}
	else
	{
		busy = 0;
	}
#endif
}

void motor_wait_handler(void)
{
	uint8_t pr;
	uint16_t ilr;

	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMB_TIMEOUT);
	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMB_TIMEOUT);

	// Stop Wait Timer
	ROM_TimerDisable(TIMER2_BASE, TIMER_B);

	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerIntClear(TIMER1_BASE, TIMER_CAPA_MATCH);
	GPIOIntClear(GPIO_PORTL_BASE, GPIO_INT_PIN_0);
	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMB_TIMEOUT);

	TimerLoadSet(TIMER1_BASE, TIMER_A, 0xFFFF);
	TimerMatchSet(TIMER1_BASE, TIMER_A, (HALF_ROTATION_COUNT*MICROSTEP)-1);

	pwm_load_idx = 0;
	pwm_load_values = &pwm_load_values_backward[0];
	pr = pwm_load_values[pwm_load_idx] >> 16;
	TimerPrescaleSet(TIMER1_BASE, TIMER_B, pr);
	ilr = pwm_load_values[pwm_load_idx] & 0xFFFFU;
	TimerLoadSet(TIMER1_BASE, TIMER_B, ilr);

	pr = pwm_load_values[pwm_load_idx] >> 17;
	TimerPrescaleMatchSet(TIMER1_BASE, TIMER_B, pr);
	ilr = (pwm_load_values[pwm_load_idx] >> 1) & 0xFFFFU;
	TimerMatchSet(TIMER1_BASE, TIMER_B, ilr);

	if (ROM_GPIOPinRead(GPIO_PORTL_BASE, GPIO_PIN_0) == GPIO_PIN_0)
	{
		homing = 1;

		// Enable homing interrupt
		GPIOIntClear(GPIO_PORTL_BASE, GPIO_INT_PIN_0);
		GPIOIntEnable(GPIO_PORTL_BASE, GPIO_INT_PIN_0);
		// Start edge count
		TimerEnable(TIMER1_BASE, TIMER_A);
		// Start PWM
		TimerEnable(TIMER1_BASE, TIMER_B);
		// Start timer to handle PWM ramp
		TimerEnable(TIMER2_BASE, TIMER_A);
	}
}

void motor_homing_handler(void)
{
	GPIOIntClear(GPIO_PORTL_BASE, GPIO_INT_PIN_0);

	// Disable homing interrupt
	GPIOIntDisable(GPIO_PORTL_BASE, GPIO_INT_PIN_0);

	// Stop Pulse count Timer
	ROM_TimerDisable(TIMER1_BASE, TIMER_A);

	// Stop PWM Timer
	ROM_TimerDisable(TIMER1_BASE, TIMER_B);

	// Stop Ramp Timer
	ROM_TimerDisable(TIMER2_BASE, TIMER_A);

	homing = 0;

	busy = 0;
}

void motor_homing(void)
{
	uint8_t pr;
	uint16_t ilr;

	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerIntClear(TIMER1_BASE, TIMER_CAPA_MATCH);
	GPIOIntClear(GPIO_PORTL_BASE, GPIO_INT_PIN_0);
	ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMB_TIMEOUT);

	TimerLoadSet(TIMER1_BASE, TIMER_A, 0xFFFF);
	TimerMatchSet(TIMER1_BASE, TIMER_A, (FULL_ROTATION_COUNT*MICROSTEP)-1);

	pwm_load_idx = 0;
	pwm_load_values = &pwm_load_values_backward[0];
	pr = pwm_load_values[pwm_load_idx] >> 16;
	TimerPrescaleSet(TIMER1_BASE, TIMER_B, pr);
	ilr = pwm_load_values[pwm_load_idx] & 0xFFFFU;
	TimerLoadSet(TIMER1_BASE, TIMER_B, ilr);

	pr = pwm_load_values[pwm_load_idx] >> 17;
	TimerPrescaleMatchSet(TIMER1_BASE, TIMER_B, pr);
	ilr = (pwm_load_values[pwm_load_idx] >> 1) & 0xFFFFU;
	TimerMatchSet(TIMER1_BASE, TIMER_B, ilr);

	TimerLoadSet(TIMER2_BASE, TIMER_A, 10000-1);

	ROM_GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_1, GPIO_PIN_1);

	homing = 1;

	if (ROM_GPIOPinRead(GPIO_PORTL_BASE, GPIO_PIN_0) == GPIO_PIN_0)
	{
		// Enable homing interrupt
		GPIOIntEnable(GPIO_PORTL_BASE, GPIO_INT_PIN_0);
		// Start edge count
		TimerEnable(TIMER1_BASE, TIMER_A);
		// Start PWM
		TimerEnable(TIMER1_BASE, TIMER_B);
		// Start timer to handle PWM ramp
		TimerEnable(TIMER2_BASE, TIMER_A);

		while (homing)
			;
	}

	// Disable homing interrupt
	GPIOIntDisable(GPIO_PORTL_BASE, GPIO_INT_PIN_0);

	// Stop Pulse count Timer
	ROM_TimerDisable(TIMER1_BASE, TIMER_A);

	// Stop PWM Timer
	ROM_TimerDisable(TIMER1_BASE, TIMER_B);

	// Stop Ramp Timer
	ROM_TimerDisable(TIMER2_BASE, TIMER_A);
}

uint8_t is_motor_busy(void)
{
	return busy;
}
