/*
 * main.c
 */

#include "driver/dbg.h"
#include "driver/ov7725.h"
#include "driver/motor.h"
#include "driver/encoder.h"
#include "driver/hardware.h"
#define TARGET_IS_TM4C129_RA1
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/timer.h"

#define I2C_DELAY_BIT_BANG		500000

#define FIXED_ENCODER_PULSES	(4300+250)
#define FIFO_SIZE				6

#define MAX_PULSES_VALUE		0xFFFF

uint8_t qvga_frame[320*240*2];

uint16_t pui16FIFO[FIFO_SIZE];
uint8_t pui8PulsesCrossOverStatus[FIFO_SIZE];
uint8_t ui8FillIndex = 0;
uint8_t ui8VacateIndex = 0;


void main(void)
{
	hw_basic_init();
	dbg_printf("Cutter Camera\r\n");

	hw_init();

	void dbg(void);
	dbg();

	while(1)
		;

}

void UpdateFIFO(void)
{
	uint8_t ui8TotalFIFOCount;
	uint16_t ui16Pulses = enc_get_latched_count();

	if(ui8FillIndex >= ui8VacateIndex)
	{
		ui8TotalFIFOCount = ui8FillIndex - ui8VacateIndex + 1;
	}
	else
	{
		ui8TotalFIFOCount = FIFO_SIZE - ui8VacateIndex + ui8FillIndex + 1;
	}

	if(ui8TotalFIFOCount < FIFO_SIZE)
	{
		pui16FIFO[ui8FillIndex] = ui16Pulses + FIXED_ENCODER_PULSES +  ov7725_get_enc_count_adjust();
		if((MAX_PULSES_VALUE - ui16Pulses) < (FIXED_ENCODER_PULSES +  ov7725_get_enc_count_adjust()) )
		{
			pui8PulsesCrossOverStatus[ui8FillIndex] = true;
		}
		else
		{
			pui8PulsesCrossOverStatus[ui8FillIndex] = false;
		}

		ui8FillIndex++;
		if(ui8FillIndex >= FIFO_SIZE)
		{
			ui8FillIndex = 0;
		}

		dbg_printf("Fill Index : %02X; Count:%d\r\n", ui8FillIndex, ov7725_get_enc_count_adjust());
	}
	else
	{
		dbg_printf("FIFO Full.\r\n");
	}
}

void CheckEncoderPulses(void)
{
	uint16_t ui16Pulses = TimerValueGet(TIMER0_BASE, TIMER_A);

	if(ui8VacateIndex != ui8FillIndex)
	{
		if(pui8PulsesCrossOverStatus[ui8VacateIndex])
		{
			if((MAX_PULSES_VALUE - ui16Pulses) < (FIXED_ENCODER_PULSES+ ov7725_get_enc_count_adjust()))
			{
				return;
			}
		}

		if(ui16Pulses >= pui16FIFO[ui8VacateIndex])
		{
			/*
			 * Start Motor
			 */
			if (!is_motor_busy())
			{
				motor_start();
			}
 			ui8VacateIndex++;
			if(ui8VacateIndex >= FIFO_SIZE)
			{
				ui8VacateIndex = 0;
			}

			dbg_printf("Vacate Index : %02X\r\n", ui8VacateIndex);
		}
	}
}

void dbg(void)
{
	int i;

	uint32_t c;
	uint8_t cmd[5];
	uint8_t wr_cnt = 0;
	uint8_t rd_cnt = 0;
	bool tmp1;
	uint16_t st1;
	uint16_t st2;

	dbg_printf("Homing.\r\n");
	motor_homing();
	dbg_printf("Ready.\r\n");
#if 1
	while(1)
	{
	 	ov7725_setup_frame_buf(qvga_frame);
		if(ov7725_detect())
		{
			UpdateFIFO();
			trigger_air_jet();
			dbg_printf("detected\r\n");
		}

		//
		// Check if Encoder Pulses has reached FIFO value
		//
		CheckEncoderPulses();
	}
#endif
//	CheckEncoderPulses();
	while (1)
	{
		uint8_t addr;
		uint8_t val;

		c = ROM_UARTCharGet(UART0_BASE);
		switch (c)
		{
			case '@':
				dbg_printf("Pulses : %X\r\n", TimerValueGet(TIMER0_BASE, TIMER_A));
				break;

			case '^':
				++wr_cnt;
				break;

			case '*':
			    ++rd_cnt;
			    break;

			case '$':
				//ov7725_dma_read_frame(qvga_frame);
				ov7725_setup_frame_buf(qvga_frame);
				while (!ov7725_is_image_acquired())
					;
				for (i = 0; i < 320*2*240; i++)
					ROM_UARTCharPut(UART0_BASE, qvga_frame[i]);
				dbg_printf("$\r\n");
				break;

			case '#':
				do
				{
				    ov7725_setup_frame_buf(qvga_frame);
				} while(!ov7725_detect());
				for (i = 0; i < 320*2*240; i++)
					ROM_UARTCharPut(UART0_BASE, qvga_frame[i]);
				dbg_printf("$\r\n");
				//UpdateFIFO();
				break;

			case '!':
				motor_start();
				break;

			case '~':
				motor_homing();
				break;

			default:
				if (wr_cnt)
				{
					cmd[wr_cnt-1] = c;
					if (++wr_cnt > 5)
					{
						if (cmd[2] != '=')
						{
							wr_cnt = 0;
						}
						else
						{
							if (cmd[0] > 0x39)
								addr = (cmd[0] - 0x41 + 0xA) << 4;
							else
								addr = (cmd[0] - 0x30) << 4;
							if (cmd[1] > 0x39)
								addr |= (cmd[1] - 0x41 + 0xA);
							else
								addr |= (cmd[1] - 0x30);

							if (cmd[3] > 0x39)
								val = (cmd[3] - 0x41 + 0xA) << 4;
							else
								val = (cmd[3] - 0x30) << 4;
							if (cmd[4] > 0x39)
								val |= (cmd[4] - 0x41 + 0xA);
							else
								val |= (cmd[4] - 0x30);

							dbg_printf("%02X=%02X\r\n", addr, val);
							ov7725_write_reg(addr, val);
							wr_cnt = 0;
							rd_cnt = 0;
						}
					}
				}
				else
				{
					wr_cnt = 0;
				}

				if (rd_cnt)
				{
				    cmd[rd_cnt-1] = c;
                    if (++rd_cnt > 3)
                    {
                        if (cmd[0] > 0x39)
                            addr = (cmd[0] - 0x41 + 0xA) << 4;
                        else
                            addr = (cmd[0] - 0x30) << 4;
                        if (cmd[1] > 0x39)
                            addr |= (cmd[1] - 0x41 + 0xA);
                        else
                            addr |= (cmd[1] - 0x30);
                        val = ov7725_read_reg(addr);
                        dbg_printf("%02X=%02X\r\n", addr, val);
                        rd_cnt = 0;
                        wr_cnt = 0;
                    }
				}
				else
				{
				    rd_cnt = 0;
				}
				break;
		}
	}
}

