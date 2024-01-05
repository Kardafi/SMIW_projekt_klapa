#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

int serwo_init(void);
int set_serwo_angle(uint32_t _serwo_angle);