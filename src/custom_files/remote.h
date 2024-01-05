#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#define BT_UUID_REMOTE_SERV_VAL \
    BT_UUID_128_ENCODE(0xe9ea0001, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)
#define BT_UUID_REMOTE_PWM_NOTIFY_VAL \
    BT_UUID_128_ENCODE(0xe9ea0002, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)
#define BT_UUID_REMOTE_PWM_WRITE_VAL \
    BT_UUID_128_ENCODE(0xe9ea0003, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)
#define BT_UUID_REMOTE_BATTERY_VAL \
    BT_UUID_128_ENCODE(0xe9ea0004, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)

#define BT_UUID_REMOTE_SERVICE          BT_UUID_DECLARE_128(BT_UUID_REMOTE_SERV_VAL)
#define BT_UUID_REMOTE_PWM_NOTIFY_CHRC     BT_UUID_DECLARE_128(BT_UUID_REMOTE_PWM_NOTIFY_VAL)
#define BT_UUID_REMOTE_PWM_WRITE_CHRC     BT_UUID_DECLARE_128(BT_UUID_REMOTE_PWM_WRITE_VAL)
#define BT_UUID_REMOTE_BATTERY_CHRC     BT_UUID_DECLARE_128(BT_UUID_REMOTE_BATTERY_VAL)

enum bt_button_notifications_enabled {
	BT_BUTTON_NOTIFICATIONS_ENABLED,
	BT_BUTTON_NOTIFICATIONS_DISABLED,
};

struct bt_remote_service_cb {
	void (*notif_changed)(enum bt_button_notifications_enabled status);
    void (*data_received)(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
};


int send_pwm_notification(struct bt_conn *conn, uint8_t value);
void set_pwm_value(uint8_t _pwm_value);
void set_battery_value(uint16_t _battery_value);
int bluetooth_init(struct bt_conn_cb * bt_cb, struct bt_remote_service_cb * remote_cb);