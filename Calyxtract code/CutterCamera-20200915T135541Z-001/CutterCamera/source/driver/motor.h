#ifndef __MOTOR_H__
#define __MOTOR_H__


#include <stdint.h>
#include <stdbool.h>

void motor_init(void);
void motor_start(void);
void motor_homing(void);
uint8_t is_motor_busy(void);

#endif /* __MOTOR_H__ */
