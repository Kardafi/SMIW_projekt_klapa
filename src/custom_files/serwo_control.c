#include "serwo_control.h"

#define PWM_PERIOD_NS 20000000 //20ms
#define MEAN_DUTY_CYCLE 1500000 //1ms srednia wartosc na poczatek (do inicjalizacji)
#define MAX_DUTY_CYCLE 2000000 //2ms
#define MIN_DUTY_CYCLE 1000000 //1ms


static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));

int serwo_init(){
        int err=0;
            if (!device_is_ready(pwm_led0.dev)) {
                return 1; //1 czyli wystapil blad
	}
        err = pwm_set_dt(&pwm_led0, PWM_PERIOD_NS, MEAN_DUTY_CYCLE); 

        return err; //jak 0 to dobrze
}

int set_serwo_angle(uint32_t duty_cycle_ns)
{
    int err=0;

    if (duty_cycle_ns > MAX_DUTY_CYCLE) //jezeli za duzo to obcinamy do max wartosci
    duty_cycle_ns=MAX_DUTY_CYCLE;
    else if (duty_cycle_ns < MIN_DUTY_CYCLE) //jezeli za malo to zwiekszamy do min wartosci
    duty_cycle_ns=MIN_DUTY_CYCLE;

    printk("SERWO zadane pwm: %"PRIu32"\n", duty_cycle_ns);
    err = pwm_set_dt(&pwm_led0, PWM_PERIOD_NS, duty_cycle_ns);

    return err;
}