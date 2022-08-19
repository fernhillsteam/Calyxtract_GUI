/**
 * ov7725.c
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
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_i2c.h"
#include "driverlib/i2c.h"
#include "driverlib/gpio.h"
#include "driverlib/udma.h"
#include "driverlib/sysctl.h"
#include "hardware.h"
#include "encoder.h"
#include "dbg.h"
#include "ov7725.h"


#define I2C_BUSY_POLL_DELAY         (200000)
#define SKIP_FRAME_COUNT            (5)
#define ORIGINAL_DETECTION_LOGIC	(1)


static uint8_t *image;
static volatile bool busy = false;
static volatile bool read_frame = false;
static volatile uint16_t line_cnt = 0;
static volatile uint16_t frame_cnt = 0;
static volatile uint16_t frame_cnt1, frame_cnt2;
static uint16_t detected_line = 0;
static volatile uint_fast8_t skip_frame_cnt;

const uint8_t config[][2] =
{
  //{0x32,0x00}, // HREF, default = 00
  //{0x2a,0x00}, // EXHCH, default = 00
  {0x11,0x02}, // CLKRC, default = 80=75Hz, 0x01=50Hz, 0x02=30Hz,0x07=8Hz(3MHz), Tested 15fps (0x02)

  {0x12,0x46}, //QVGA RGB565, default = 00
  //{0x12,0x46},
  //{0x13,0xFF},//Default = 8F
  //{0x69,0x5D},

  /*
  {0x12,0x42}, //QVGA GRB422, default = 00
  {0x12,0x42},
*/
  {0x15,0x20},
#if 1
  {0x0C,0x10},
#else
  {0x0C,0x11},//test pattern
#endif
#if 0 == 1
  {0x42,0x7f}, // Blue Channel target gain, default = 80
  {0x4d,0x00},//0x09 // FixGain, default = 00
  {0x63,0xf0}, // AWB_Ctrl0, default = F0
  {0x64,0xff}, // DSP_Ctrl1, default = 1F
  {0x65,0x20}, // DSP_Ctrl2, default = 00
 {0x66,0x00}, // DSP_Ctrl3, default = 10
  {0x67,0x00}, // DSP_Ctrl4, default = 00
  {0x69,0x5d}, // AWB_Ctrl, default = 5C
#endif
#if 0 == 1
  {0x22,0xFF},//7f // BDBase, default = FF
  {0x23,0x01}, // BDMStep, default = 00
  {0x24,0x34}, // AEW, default = 76
  {0x25,0x3c}, // AEB, default = 63
  {0x26,0xa1}, // VPT, default = D4
  {0x2b,0x00}, // EXHCL, default = 00
  {0x6b,0xaa}, // AWBCtrl3, default = A2
#endif

#if 0 == 1
  {0x90,0x0a},// // EDGE1, default = 08
  {0x91,0x01},// // DNSOff, default = 10
  {0x92,0x01},// // EDGE2, default = 1F
  {0x93,0x01}, // EDGE3, default = 01
#endif

#if 0 == 1
  {0x94,0x5f}, // MTX1, default = 2C
  {0x95,0x53}, // MTX2, default = 24
  {0x96,0x11}, // MTX3, default = 08
  {0x97,0x1a}, // MTX4, default = 14
  {0x98,0x3d}, // MTX5, default = 24
  {0x99,0x5a}, // MTX6, default = 38
  {0x9a,0x1e}, // MTX_Ctrl, default = 9E
#endif
#if 0 == 0
    {0x9b,0x20},//set luma, default = 00
	//{0x9c,0x80},//25//set contrast, default = 40
	//{0xa7,0x80},//set saturation, default = 40
	//{0xa8,0x80},//set saturation, default = 40
	//{0xa9,0x80},//set hue, default = 80
	//{0xaa,0x80},//set hue, default = 80
#endif
#if 0 == 0
	//{0x9e,0x81}, // UVADJ0, default = 11
	{0xa6,0x06}, // SDE, default = 00
#endif
#if 0 == 1
	{0x7e,0x0c}, // GAM1, default = 0E
	{0x7f,0x16}, // GAM2, default = 1A
	{0x80,0x2a}, // GAM3, default = 31
	{0x81,0x4e}, // GAM4, default = 5A
	{0x82,0x61}, // GAM5, default = 69
	{0x83,0x6f}, // GAM6, default = 75
	{0x84,0x7b}, // GAM7, default = 7E
	{0x85,0x86}, // GAM8, default = 88
	{0x86,0x8e}, // GAM9, default = 8F
	{0x87,0x97}, // GAM10, default = 96
	{0x88,0xa4}, // GAM11, default = A3
	{0x89,0xaf}, // GAM12, default = AF
	{0x8a,0xc5}, // GAM13, default = C4
	{0x8b,0xd7}, // GAM14, default = D7
	{0x8c,0xe8}, // GAM15, default = E8
	{0x8d,0x20}, // SLOP, default = 20
#endif

#if 0 == 1
	{0x33,0x00}, // Dummy Rows, default = 00
	{0x22,0x99}, // BDBase, default = FF
	{0x23,0x03}, // BDMBase, dfeault = 01
	{0x4a,0x00}, // LC_RADI, default =30
	{0x49,0x13}, // LC_COEFF, default = 50
	{0x47,0x08}, // LC_YC, default = 00
	{0x4b,0x14}, // LC_COEFFB, default = 50
	{0x4c,0x17}, // LC_COEFFR, default = 50
	{0x46,0x05}, // LC_CTR, default = 00
#endif

#if 0

  {0x42,0x7f}, // Blue Channel target gain, default = 80
  {0x4d,0x00},//0x09 // FixGain, default = 00
  {0x63,0xf0}, // AWB_Ctrl0, default = F0
  {0x64,0xff}, // DSP_Ctrl1, default = 1F
  {0x65,0x20}, // DSP_Ctrl2, default = 00
  {0x66,0x00}, // DSP_Ctrl3, default = 10
  {0x67,0x00}, // DSP_Ctrl4, default = 00
  {0x69,0x5d}, // AWB_Ctrl, default = 5C


  {0x13,0xff}, // COM8, default = 8F
/*  {0x0d,0x61},//PLL*/
  {0x0f,0xc5}, // COM6, default = 43
  {0x14,0x11}, // COM9, default = 4A
  {0x22,0xFF},//7f // BDBase, default = FF
  {0x23,0x01}, // BDMStep, default = 00
  {0x24,0x34}, // AEW, default = 76
  {0x25,0x3c}, // AEB, default = 63
  {0x26,0xa1}, // VPT, default = D4
  {0x2b,0x00}, // EXHCL, default = 00
  {0x6b,0xaa}, // AWBCtrl3, default = A2
  {0x13,0xff}, // COM8, default = 8F

  {0x90,0x0a},// // EDGE1, default = 08
  {0x91,0x01},// // DNSOff, default = 10
  {0x92,0x01},// // EDGE2, default = 1F
  {0x93,0x01}, // EDGE3, default = 01

  {0x94,0x5f}, // MTX1, default = 2C
  {0x95,0x53}, // MTX2, default = 24
  {0x96,0x11}, // MTX3, default = 08
  {0x97,0x1a}, // MTX4, default = 14
  {0x98,0x3d}, // MTX5, default = 24
  {0x99,0x5a}, // MTX6, default = 38
  {0x9a,0x1e}, // MTX_Ctrl, default = 9E

  {0x9b,0x00},//set luma, default = 00
  {0x9c,0x25},//set contrast, default = 40
  {0xa7,0x65},//set saturation, default = 40
  {0xa8,0x65},//set saturation, default = 40
  {0xa9,0x80},//set hue, default = 80
  {0xaa,0x80},//set hue, default = 80

  {0x9e,0x81}, // UVADJ0, default = 11
  {0xa6,0x06}, // SDE, default = 00

  {0x7e,0x0c}, // GAM1, default = 0E
  {0x7f,0x16}, // GAM2, default = 1A
  {0x80,0x2a}, // GAM3, default = 31
  {0x81,0x4e}, // GAM4, default = 5A
  {0x82,0x61}, // GAM5, default = 69
  {0x83,0x6f}, // GAM6, default = 75
  {0x84,0x7b}, // GAM7, default = 7E
  {0x85,0x86}, // GAM8, default = 88
  {0x86,0x8e}, // GAM9, default = 8F
  {0x87,0x97}, // GAM10, default = 96
  {0x88,0xa4}, // GAM11, default = A3
  {0x89,0xaf}, // GAM12, default = AF
  {0x8a,0xc5}, // GAM13, default = C4
  {0x8b,0xd7}, // GAM14, default = D7
  {0x8c,0xe8}, // GAM15, default = E8
  {0x8d,0x20}, // SLOP, default = 20

  {0x33,0x00}, // Dummy Rows, default = 00
  {0x22,0x99}, // BDBase, default = FF
  {0x23,0x03}, // BDMBase, dfeault = 01
  {0x4a,0x00}, // LC_RADI, default =30
  {0x49,0x13}, // LC_COEFF, default = 50
  {0x47,0x08}, // LC_YC, default = 00
  {0x4b,0x14}, // LC_COEFFB, default = 50
  {0x4c,0x17}, // LC_COEFFR, default = 50
  {0x46,0x05}, // LC_CTR, default = 00
  {0x0e,0xf5}, // COM5, default = 01
  {0x0c,0xd0}, // COM3, default = 10

#endif
  {0x08,0x01},  // LB, exposure gain
  {0x09,0x03},  // COM2, Default = 01
  {0x10,0x05},  // HB, exposure gain
  {0x13,0x00},  // COM8, Default = CF to disable AGC, AWB and AEC
  {0x63,0x7f},  // AWB gain disable, default = 70 in data-sheet but FF should be used
  {0x64,0xb8},
  {0x69,0x54},
  {0x00,0xf5},
  {0xff,0xff}
  };

static const uint16_t pixel_to_enc_count[240] =
{
	 0,   1,   2,   4,   5,   6,   8,   9,  10,  11,  12,  14,  15,  16,  18,  19,
	20,  21,  22,  24,  25,  26,  28,  29,  30,  31,  32,  34,  35,  36,  38,  39,
	40,  41,  42,  44,  45,  46,  48,  49,  50,  51,  52,  54,  55,  56,  58,  59,
	60,  61,  62,  64,  65,  66,  68,  69,  70,  71,  72,  74,  75,  76,  78,  79,
	80,  81,  82,  84,  85,  86,  88,  89,  90,  91,  92,  94,  95,  96,  98,  99,
	100, 101, 102, 104, 105, 106, 108, 109, 110, 111, 112, 114, 115, 116, 118, 119,
	120, 121, 122, 124, 125, 126, 128, 129, 130, 131, 132, 134, 135, 136, 138, 139,
	140, 141, 142, 144, 145, 146, 148, 149, 150, 151, 152, 154, 155, 156, 158, 159,
	160, 161, 162, 164, 165, 166, 168, 169, 170, 171, 172, 174, 175, 176, 178, 179,
	180, 181, 182, 184, 185, 186, 188, 189, 190, 191, 192, 194, 195, 196, 198, 199,
	200, 201, 202, 204, 205, 206, 208, 209, 210, 211, 212, 214, 215, 216, 218, 219,
	220, 221, 222, 224, 225, 226, 228, 229, 230, 231, 232, 234, 235, 236, 238, 239,
	240, 241, 242, 244, 245, 246, 248, 249, 250, 251, 252, 254, 255, 256, 258, 259,
	260, 261, 262, 264, 265, 266, 268, 269, 270, 271, 272, 274, 275, 276, 278, 279,
	280, 281, 282, 284, 285, 286, 288, 289, 290, 291, 292, 294, 295, 296, 298, 299,
};

void ov7725_reset(bool en)
{
	if (en)
		GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_0, 0);
	else
		GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_0, GPIO_PIN_0);
}

void ov7725_pwr_dn(bool en)
{
	if (en)
		GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_1, GPIO_PIN_1);
	else
		GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_1, 0);
}

void ov7725_init(void)
{
	ov7725_pwr_dn(true);
	ov7725_reset(true);

	SysCtlDelay(500000*200);

	ov7725_pwr_dn(false);
	ov7725_reset(false);

	SysCtlDelay(500000*200);

	ov7725_write_reg(0x12, 0x80);

	SysCtlDelay(500000*200);

	int i = 0;
	uint8_t tmp;

	dbg_printf("OV7725: Initializing ...\r\n");
	while (config[i][0] != 0xFF)
	{
		//hw_delay(I2C_DELAY_BIT_BANG*10);
		tmp = ov7725_write_reg(config[i][0], config[i][1]);
		tmp = ov7725_read_reg(config[i][0]);
		dbg_printf("%02X=%02X\r\n", config[i][0], tmp);
		if (tmp == config[i][1])
			++i;
	}
	dbg_printf("OV7725: Init done.\r\n");
}

uint8_t ov7725_read_reg(uint8_t reg)
{
	uint32_t err;

	SysCtlDelay(4000000);

	while(I2CMasterBusy(I2C5_BASE))
		;

	I2CMasterSlaveAddrSet(I2C5_BASE, 0x21, false);

	I2CMasterDataPut(I2C5_BASE, reg);

	I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_SINGLE_SEND);

	//SysCtlDelay(I2C_BUSY_POLL_DELAY);
	while(!I2CMasterBusy(I2C5_BASE))
		;

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

	//SysCtlDelay(I2C_BUSY_POLL_DELAY);
	while(!I2CMasterBusy(I2C5_BASE))
		;

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

uint8_t ov7725_write_reg(uint8_t reg, uint8_t value)
{
	uint32_t err;

	SysCtlDelay(4000000);

	while(I2CMasterBusy(I2C5_BASE))
		;

	I2CMasterSlaveAddrSet(I2C5_BASE, 0x21, false);

	I2CMasterDataPut(I2C5_BASE, reg);

	I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_BURST_SEND_START);

	//SysCtlDelay(I2C_BUSY_POLL_DELAY);
	while(!I2CMasterBusy(I2C5_BASE))
		;

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

	//SysCtlDelay(I2C_BUSY_POLL_DELAY);
	while(!I2CMasterBusy(I2C5_BASE))
		;

	while(I2CMasterBusy(I2C5_BASE))
		;

	err = I2CMasterErr(I2C5_BASE);

	if (err)
	{
		dbg_printf("Err = %08X\r\n", err);
		return 0;
	}

	I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

	//SysCtlDelay(I2C_BUSY_POLL_DELAY);
	while(!I2CMasterBusy(I2C5_BASE))
		;

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

bool ov7725_setup_frame_buf(uint8_t *img)
{
	if (read_frame || busy)
		return false;

	image = img;
	busy = true;

	return true;
}

bool ov7725_is_image_acquired(void)
{
	return (read_frame || busy);
}

bool ov7725_detect(void)
{
	uint16_t processed_line = 0;
	uint16_t cnt;
	uint16_t tmp;
	uint8_t r, g, b;
	uint16_t white_count;
	uint16_t row_count = 0;
	uint16_t first_white_sum_detected;
	bool ret = false;

	while(!read_frame)
		;

	enc_latch_count();
	while (processed_line < 240)
	{
		if (processed_line != line_cnt)
		{
		    white_count = 0;
			for (cnt = 0; cnt < 320*2; cnt+=2)
			{
				tmp = __rev16(*(uint16_t *)&image[320*2*processed_line+cnt]);
#if ORIGINAL_DETECTION_LOGIC
				r = (tmp >> 11) & 0x1F;
				g = (tmp >> 5) & 0x3F;
				b = tmp & 0x1F;
#else
				r = (tmp >> 11) & 0x1F;
				r <<= 3;
				g = (tmp >> 5) & 0x3F;
				g <<= 2;
				b = tmp & 0x1F;
				b <<= 3;
#endif

#if ORIGINAL_DETECTION_LOGIC
				if ((b < 14) && (g > 25) && (r < 14) && (processed_line < 200) && (processed_line > 35))
#else
				if ((b < 100) && (r < 100) && (g > 70) && ((g - b) > 5) && ((g - r) > 5) && (processed_line > 35) && (processed_line < 180))
#endif
				{
				    ++white_count;
				    //*(uint16_t *)&image[320*2*processed_line+cnt] = 0xFFFF;
				}
				else
				{
					//*(uint16_t *)&image[320*2*processed_line+cnt] = 0;
				}
			}
#if ORIGINAL_DETECTION_LOGIC
			if ((white_count > 5)/* && (white_count < 35)*/)
#else
			if ((white_count > 20) && (white_count < 30))
#endif
			{
			    ++row_count;
			}
#if !ORIGINAL_DETECTION_LOGIC
			if (row_count == 1)
			{
				first_white_sum_detected = processed_line;
			}
#endif
			if(row_count == 3)
			{
#if ORIGINAL_DETECTION_LOGIC
			    detected_line = processed_line;
#else
			    detected_line = first_white_sum_detected;
#endif
			}

			++processed_line;
		}
	}
	if (row_count > 5)
	    ret = true;

//	if (ret)
//	{
//		int i;
//		for (i = 0; i < 320; i++)
//		{
//			*(uint16_t *)&image[320*2*detected_line+i] = 0xFFFF;
//		}
//	}

	return ret;
}

bool ov7725_detect1(void)
{
	uint16_t processed_line = 0;
	uint16_t cnt;
	uint16_t tmp;
	uint8_t /*r,*/ g, b;
	uint16_t white_count;
	uint16_t row_count = 0;
	bool ret = false;

	if(!read_frame)
	{
		return ret;
	}
	enc_latch_count();

	while (processed_line < 240)
	{
		if (processed_line != line_cnt)
		{
		    white_count = 0;
			for (cnt = 0; cnt < 320*2; cnt+=2)
			{
				tmp = __rev16(*(uint16_t *)&image[320*2*processed_line+cnt]);
				//g = (tmp >> 5) & 0x3F;
				g = (tmp >> 5) & 0x3F;
				b = tmp & 0x1F;

				if (b < 14 && g > 25)
				    ++white_count;
			}
			if ((white_count > 10)/* && (white_count < 35)*/)
			    ++row_count;

			++processed_line;
		}
	}
	if (row_count > 5)
	{
	    ret = true;
	    skip_frame_cnt = SKIP_FRAME_COUNT;
	}

	return ret;
}

uint16_t ov7725_get_enc_count_adjust(void)
{
	if (detected_line < 240)
		return pixel_to_enc_count[240-detected_line-1];
	else
		return 0;
}

void ov7725_get_frame_cnt(uint16_t *frame1, uint16_t *frame2)
{
	*frame1 = frame_cnt1;
	*frame2 = frame_cnt2;
}

void vsync_handler(void)
{
	GPIOIntClear(GPIO_PORTP_BASE, GPIO_INT_PIN_0);
	++frame_cnt;
	frame_cnt1 = frame_cnt;

	if (skip_frame_cnt)
	{
		--skip_frame_cnt;
	}
	else
	{
		if (busy)
		{
			line_cnt = 0;
			hw_dma_set_img(&image[320*2*line_cnt]);
			busy = false;
			read_frame = true;
		}
	}
}

void href_handler(void)
{
	GPIOIntClear(GPIO_PORTQ_BASE, GPIO_INT_PIN_0);
	if (read_frame)
	{
		if (++line_cnt < 240)
		{
			hw_dma_set_img(&image[320*2*line_cnt]);
		}
		else
		{
			read_frame = false;
		}
	}
}
