#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "serwo_control.h"

#include "remote.h"

static struct bt_conn *current_conn;

/* LEDs, BUTTON, USB*/
#define LED0_NODE	DT_ALIAS(led0)
static const struct gpio_dt_spec led_button = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE	DT_ALIAS(led1_blue)
static const struct gpio_dt_spec led_bluetooth = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

#define SW0_NODE	DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
uint32_t dtr = 0;

/* Declarations */
#define SLEEP_TIME_MS   10*60*1000 /*the sleep time 10 minutes  */

#define MEAN_DUTY_CYCLE       1500000
#define MAX_DUTY_CYCLE        2000000
#define MIN_DUTY_CYCLE        1000000
volatile int serwo_angle    = MEAN_DUTY_CYCLE;

void on_connected(struct bt_conn *conn, uint8_t err);
void on_disconnected(struct bt_conn *conn, uint8_t reason);
void on_notif_changed(enum bt_button_notifications_enabled status);
void on_data_received(struct bt_conn *conn, const uint16_t *const data, uint16_t len);

static struct gpio_callback button_cb_data;

struct bt_conn_cb bluetooth_callbacks = {
    .connected      = on_connected,
    .disconnected   = on_disconnected,
};

struct bt_remote_service_cb remote_callbacks = {
	.notif_changed = on_notif_changed,
    .data_received = on_data_received,
};

/* Callbacks */

void on_data_received(struct bt_conn *conn, const uint16_t *const data, uint16_t len)
{
    uint16_t temp_str[len+1];
    memcpy(temp_str, data, len);
    temp_str[len] = 0x00;

	gpio_pin_toggle_dt(&led_button);
    printk("Received data on conn %p. Len: %d\n", (void *)conn, len);
    printk("Data: %"PRIu16"\n", temp_str[0]);

    //mapowanie na potrzeby otrzymania wartosci uint32_t
	//1 000 000ms/65535 = 15.2590218967
	//uint32_t temp = (uint32_t)data[0]*16;

	uint32_t temp_pwm = 1000000 + (temp_str[0]*16);
	serwo_angle=(temp_pwm);
	set_serwo_angle(temp_pwm);
}

void on_notif_changed(enum bt_button_notifications_enabled status)
{
    if (status == BT_BUTTON_NOTIFICATIONS_ENABLED) {
        printk("Notifications enabled (on_notif_changed)\n");
    } else {
        printk("Notifications disabled (on_notif_changed)\n");
    }
}

void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("connection failed, err %d\n", err);
    }
    printk("Connected to central\n");
    current_conn = bt_conn_ref(conn);
    gpio_pin_set_dt(&led_bluetooth,1); //dioda niebieska swieci
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason: %d)\n", reason);
	gpio_pin_set_dt(&led_bluetooth,0); //dioda niebieska gasnie
	if(current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_toggle_dt(&led_button);
	serwo_angle+=200000; //+0,2ms

    if (serwo_angle > 2000000){
		serwo_angle = 1000000;
	}
        
	printk("BUTTON pwm: %"PRIu32"\n", serwo_angle);

    set_serwo_angle(serwo_angle);
	set_pwm_value(serwo_angle); //dla bluetooth
	send_pwm_notification(current_conn, serwo_angle);
}

/* Configurations */
int leds_usb_button_init(){
	int ret=0;

	//USB
	if (usb_enable(NULL)) {
        return -1;
	}
	//bez while bo bedziemy czekac w nieskonczonosc jezeli nie wlaczymy serial monitora
	//while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(1000));
	//}
	printk("Sterownik klapy. Inicjalizacja...\n");

	//LED BLUETOOTH
	if (!device_is_ready(led_bluetooth.port)) {
		return -1;
	}
	ret = gpio_pin_configure_dt(&led_bluetooth, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		return -1;
	}
	gpio_pin_set_dt(&led_bluetooth,0); //0 to wylaczona

    //LED BUTTON
	if (!device_is_ready(led_button.port)) {
		return -1;
	}
	ret = gpio_pin_configure_dt(&led_button, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}
	gpio_pin_set_dt(&led_button,1); //1 to wlaczona
	
	//BUTTON
	if (!device_is_ready(button.port)) {
		return -1;
	}
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return -1;
	}
	/* STEP 3 - Configure the interrupt on the button's pin */
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE );

	/* STEP 6 - Initialize the static struct gpio_callback variable   */
    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin)); 	
	
	/* STEP 7 - Add the callback function by calling gpio_add_callback()   */
	gpio_add_callback(button.port, &button_cb_data);
	
	return ret;
}

int main(void)
{
    int err=0;
	err = leds_usb_button_init();
	err = serwo_init();
    err = bluetooth_init(&bluetooth_callbacks,&remote_callbacks);
  
    if (err) {
        printk("Init failed (err %d)\n", err);
    }

	printk("Poprawnie uruchomiono! %s\n", CONFIG_BOARD);

    for (;;) {
        k_msleep(SLEEP_TIME_MS);
    }
}