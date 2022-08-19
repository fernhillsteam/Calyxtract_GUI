/**
 * ov7670.c
 *
 *  Created on: 10-Oct-2016
 *      Author: Jayanth S. Moodliar
 *
 * (c) TechnoConfluence, 2016
 */

#include <stdint.h>
#include <stdbool.h>
#define TARGET_IS_TM4C129_RA1
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_i2c.h"
#include "driverlib/i2c.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "dbg.h"
#include "ov7670.h"


void ov7670_reset(bool en)
{
	if (en)
		GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_0, 0);
	else
		GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_0, GPIO_PIN_0);
}

void ov7670_pwr_dn(bool en)
{
	if (en)
		GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_1, GPIO_PIN_1);
	else
		GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_1, 0);
}

void ov7670_init(void)
{
	ov7670_pwr_dn(false);
	ov7670_reset(false);

	SysCtlDelay(500000*200);



}

uint8_t ov7670_read_reg(uint8_t reg)
{
	uint32_t err;

	SysCtlDelay(2000000);

	while(I2CMasterBusy(I2C5_BASE))
		;

	I2CMasterSlaveAddrSet(I2C5_BASE, 0x21, false);

	I2CMasterDataPut(I2C5_BASE, reg);

	I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_SINGLE_SEND);

	SysCtlDelay(20000);

	while(I2CMasterBusy(I2C5_BASE))
		;

	err = I2CMasterErr(I2C5_BASE);

	if (err)
	{
		dbg_printf("Err = %08X\r\n", err);
		return 0;
	}

	I2CMasterSlaveAddrSet(I2C5_BASE, 0x21, true);

	I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);

	SysCtlDelay(20000);

	while(I2CMasterBusy(I2C5_BASE))
		;

	err = I2CMasterErr(I2C5_BASE);

	if (err)
	{
		dbg_printf("Err = %08X\r\n", err);
		return 0;
	}

	return I2CMasterDataGet(I2C5_BASE);
}

uint8_t ov7670_write_reg(uint8_t reg, uint8_t value)
{
	uint32_t err;

	SysCtlDelay(2000000);

	while(I2CMasterBusy(I2C5_BASE))
		;

	I2CMasterSlaveAddrSet(I2C5_BASE, 0x21, false);

	I2CMasterDataPut(I2C5_BASE, reg);

	I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_BURST_SEND_START);

	SysCtlDelay(20000);

	while(I2CMasterBusy(I2C5_BASE))
		;

	err = I2CMasterErr(I2C5_BASE);

	if (err)
	{
		dbg_printf("Err = %08X\r\n", err);
		return 0;
	}

	I2CMasterDataPut(I2C5_BASE, value);

	I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);

	SysCtlDelay(20000);

	while(I2CMasterBusy(I2C5_BASE))
		;

	err = I2CMasterErr(I2C5_BASE);

	if (err)
	{
		dbg_printf("Err = %08X\r\n", err);
		return 0;
	}

	I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

	SysCtlDelay(20000);

	while(I2CMasterBusy(I2C5_BASE))
		;

	err = I2CMasterErr(I2C5_BASE);

	if (err)
	{
		dbg_printf("Err = %08X\r\n", err);
		return 0;
	}

	return err;
}

void ov7670_read_frame(uint16_t *img)
{
	uint16_t tmp;
	int i;

	i = 0;
	while((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == GPIO_PIN_0)
		;

	while((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0)
		;

	while((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0)
	{
		while(((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0) && ((GPIOPinRead(GPIO_PORTQ_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0))
			;

		if ((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == GPIO_PIN_0)
			break;

		while((GPIOPinRead(GPIO_PORTQ_BASE, GPIO_PIN_0) & GPIO_PIN_0) == GPIO_PIN_0)
		{
			while((GPIOPinRead(GPIO_PORTK_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0)
				;

			tmp = GPIOPinRead(GPIO_PORTM_BASE, 0xFFU);
			tmp <<= 8;

			while((GPIOPinRead(GPIO_PORTK_BASE, GPIO_PIN_0) & GPIO_PIN_0) == GPIO_PIN_0)
				;


			while((GPIOPinRead(GPIO_PORTK_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0)
				;

			tmp |= GPIOPinRead(GPIO_PORTM_BASE, 0xFFU);

			while((GPIOPinRead(GPIO_PORTK_BASE, GPIO_PIN_0) & GPIO_PIN_0) == GPIO_PIN_0)
				;

			img[i] = tmp;
			++i;
			if (i > 176*120)
				i = 176*120;
		}

	}
}

void cnt_vsync(void)
{
	uint16_t vsync;
	uint16_t href;

	vsync = 0;

	while(1)
	{
		while((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0)
			;

		while((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == GPIO_PIN_0)
			;

		href = 0;;
		while((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0)
		{
			while(((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0) && ((GPIOPinRead(GPIO_PORTQ_BASE, GPIO_PIN_0) & GPIO_PIN_0) == 0))
				;

			if ((GPIOPinRead(GPIO_PORTP_BASE, GPIO_PIN_0) & GPIO_PIN_0) == GPIO_PIN_0)
				break;


			while((GPIOPinRead(GPIO_PORTQ_BASE, GPIO_PIN_0) & GPIO_PIN_0) == GPIO_PIN_0)
				;
			href++;
		}
		vsync++;
	}
}
