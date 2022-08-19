#ifndef __HARDWARE_H__
#define __HARDWARE_H__


#include <stdint.h>
#include <stdbool.h>

void hw_init(void);
void hw_basic_init(void);
void led1_on(void);
void led1_off(void);
void led2_on(void);
void led2_off(void);
void hw_delay(uint32_t cnt);
void hw_all_interrupt_enable(bool en);
void hw_dma_set_img(uint8_t *p_img);
uint8_t hw_is_dma_img_complete(void);
void trigger_air_jet(void);

#endif /* __HARDWARE_H__ */
