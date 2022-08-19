#ifndef __ENCODER_H__
#define __ENCODER_H__


#include <stdint.h>
#include <stdbool.h>

void enc_init(void);
void enc_latch_count(void);
uint32_t enc_get_latched_count(void);
void enc_flag_detection(void);


#endif /* __MOTOR_H__ */
