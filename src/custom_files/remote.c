#include "remote.h"

static K_SEM_DEFINE(bt_init_ok, 0, 1);
static uint16_t pwm_value = 0;
static struct bt_remote_service_cb remote_service_callbacks;

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME)-1)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_REMOTE_SERV_VAL),
};

/* Declarations */
static ssize_t read_pwm_characteristic_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);
void pwm_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static ssize_t on_write(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags);


BT_GATT_SERVICE_DEFINE(remote_srv,
BT_GATT_PRIMARY_SERVICE(BT_UUID_REMOTE_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_REMOTE_PWM_CHRC,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_pwm_characteristic_cb, NULL, NULL),
    BT_GATT_CCC(pwm_chrc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_REMOTE_MESSAGE_CHRC,
                    BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                    BT_GATT_PERM_WRITE,
                    NULL, on_write, NULL),
);

/* Callbacks */

static ssize_t on_write(struct bt_conn *conn,
              const struct bt_gatt_attr *attr,
              const void *buf,
              uint16_t len,
              uint16_t offset, 
              uint8_t flags)
{
    printk("Received data, handle %d, conn %p\n",attr->handle, (void *)conn);

    if (remote_service_callbacks.data_received) {
        remote_service_callbacks.data_received(conn, buf, len);
    }
    return len;
}

void on_sent(struct bt_conn *conn, void *user_data)
{
    ARG_UNUSED(user_data);
    printk("Notification sent on connection %p\n", (void *)conn);
}

void pwm_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    printk("Notifications %s", notif_enabled? "enabled\n":"disabled\n");
    if (remote_service_callbacks.notif_changed) {
        remote_service_callbacks.notif_changed(notif_enabled?BT_BUTTON_NOTIFICATIONS_ENABLED:BT_BUTTON_NOTIFICATIONS_DISABLED);
    }
}

static ssize_t read_pwm_characteristic_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &pwm_value,
				 sizeof(pwm_value));
}

void bluetooth_ready_callback(int err)
{
    if (err)
    {
        printk("bluetooth_ready_callback err %d\n", err);
    }
    k_sem_give(&bt_init_ok);
}

/* Remote service functions */

int send_pwm_notification(struct bt_conn *conn, uint16_t value)
{
    int err = 0;

    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &remote_srv.attrs[2];

    params.attr = attr;
    params.data = &value;
    params.len = 2;
    params.func = NULL;//on_sent;

    err= bt_gatt_notify(conn,attr,&value,sizeof(value));
    //err = bt_gatt_notify_cb(conn, &params);

    return err;
}

void set_pwm_value(uint16_t _pwm_value)
{
    pwm_value = _pwm_value;
}

int bluetooth_init(struct bt_conn_cb * bt_cb, struct bt_remote_service_cb * remote_cb)
{
    int err = 0;
    printk("Initializing Bluetooth\n");

    if (bt_cb == NULL || remote_cb == NULL){
        return -1;
    }

    {
        bt_conn_cb_register(bt_cb);
        remote_service_callbacks.notif_changed = remote_cb->notif_changed;
        remote_service_callbacks.data_received = remote_cb->data_received;
    }
    
    err = bt_enable(bluetooth_ready_callback);
    if (err) {
        printk("bt_enable() ret %d\n", err);
        return err;
    }
    k_sem_take(&bt_init_ok, K_FOREVER);

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Couldn't start advertising. (err %d)\n", err);
        return err;
    }

    return err;
}