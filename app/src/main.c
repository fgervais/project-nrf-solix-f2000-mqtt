#include <app_event_manager.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/debug/thread_analyzer.h>

#include <zephyr/bluetooth/bluetooth.h>
// #include <zephyr/bluetooth/hci.h>
// #include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
// #include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#define MODULE main
#include <caf/events/module_state_event.h>
#include <caf/events/button_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <app_version.h>
#include <mymodule/base/reset.h>
#include <mymodule/base/watchdog.h>


#define BUTTON_PRESS_EVENT		BIT(0)

// #define ADV_NAME_STR_MAX_LEN (sizeof(CONFIG_BT_DEVICE_NAME))
#define ADV_NAME_STR_MAX_LEN 64


static K_EVENT_DEFINE(button_events);


static struct bt_conn *default_conn = NULL;

// notify_char = "00008888-0000-1000-8000-00805f9b34fb"
// write_char = "00007777-0000-1000-8000-00805f9b34fb"

// static const struct bt_uuid *service_uuid = BT_UUID_DECLARE_128(
// 	BT_UUID_128_ENCODE(0x00008888, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb));

// static const struct bt_uuid *service_uuid = BT_UUID_DECLARE_128(
// 	BT_UUID_128_ENCODE(0x8c850003, 0x0302, 0x41c5, 0xb46e, 0xcf057c562025));

// static const struct bt_uuid *service_uuid = BT_UUID_DECLARE_16(0x8888);

static const struct bt_uuid *service_uuid = BT_UUID_DECLARE_16(0x2902);

static struct bt_gatt_subscribe_params subscribe_params;
static struct bt_gatt_write_params write_params;
static struct bt_gatt_exchange_params exchange_params;

// static uint8_t write_data[] = {
// 	0xff, 0x09, 0x36, 0x00, 0x03, 0x00, 0x01, 0x00,
// 	0x01, 0xa1, 0x04, 0x36, 0x12, 0x68, 0x67, 0xa2,
// 	0x24, 0x64, 0x35, 0x38, 0x66, 0x36, 0x38, 0x30,
// 	0x65, 0x2d, 0x32, 0x39, 0x34, 0x37, 0x2d, 0x34,
// 	0x33, 0x32, 0x39, 0x2d, 0x62, 0x39, 0x63, 0x62,
// 	0x2d, 0x63, 0x39, 0x35, 0x32, 0x30, 0x30, 0x35,
// 	0x34, 0x36, 0x34, 0x61, 0x35, 0xcb
// };

static uint8_t write_data1[] = {
	0xff, 0x09, 0x36, 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0xa1, 0x04, 0xfd, 0x72, 0x6d, 0x67, 0xa2,
	0x24, 0x64, 0x35, 0x38, 0x66, 0x36, 0x38, 0x30, 0x65, 0x2d, 0x32, 0x39, 0x34, 0x37, 0x2d, 0x34,
	0x33, 0x32, 0x39, 0x2d, 0x62, 0x39, 0x63, 0x62, 0x2d, 0x63, 0x39, 0x35, 0x32, 0x30, 0x30, 0x35,
	0x34, 0x36, 0x34, 0x61, 0x35, 0x65,
};

static uint8_t write_data2[] = {
	0xff, 0x09, 0x3d, 0x00, 0x03, 0x00, 0x01, 0x00, 0x03, 0xa1, 0x04, 0xfd, 0x72, 0x6d, 0x67, 0xa2,
	0x24, 0x64, 0x35, 0x38, 0x66, 0x36, 0x38, 0x30, 0x65, 0x2d, 0x32, 0x39, 0x34, 0x37, 0x2d, 0x34,
	0x33, 0x32, 0x39, 0x2d, 0x62, 0x39, 0x63, 0x62, 0x2d, 0x63, 0x39, 0x35, 0x32, 0x30, 0x30, 0x35,
	0x34, 0x36, 0x34, 0x61, 0x35, 0xa3, 0x01, 0x20, 0xa4, 0x02, 0x00, 0xf0, 0xb8,
};

static uint8_t write_data3[] = {
	0xff, 0x09, 0x36, 0x00, 0x03, 0x00, 0x01, 0x00, 0x29, 0xa1, 0x04, 0xfd, 0x72, 0x6d, 0x67, 0xa2,
	0x24, 0x64, 0x35, 0x38, 0x66, 0x36, 0x38, 0x30, 0x65, 0x2d, 0x32, 0x39, 0x34, 0x37, 0x2d, 0x34,
	0x33, 0x32, 0x39, 0x2d, 0x62, 0x39, 0x63, 0x62, 0x2d, 0x63, 0x39, 0x35, 0x32, 0x30, 0x30, 0x35,
	0x34, 0x36, 0x34, 0x61, 0x35, 0x4d,
};

static uint8_t write_data4[] = {
	0xff, 0x09, 0x40, 0x00, 0x03, 0x00, 0x01, 0x00, 0x05, 0xa1, 0x04, 0xfd, 0x72, 0x6d, 0x67, 0xa2,
	0x24, 0x64, 0x35, 0x38, 0x66, 0x36, 0x38, 0x30, 0x65, 0x2d, 0x32, 0x39, 0x34, 0x37, 0x2d, 0x34,
	0x33, 0x32, 0x39, 0x2d, 0x62, 0x39, 0x63, 0x62, 0x2d, 0x63, 0x39, 0x35, 0x32, 0x30, 0x30, 0x35,
	0x34, 0x36, 0x34, 0x61, 0x35, 0xa3, 0x01, 0x20, 0xa4, 0x02, 0x00, 0xf0, 0xa5, 0x01, 0x02, 0x65,
};

static uint8_t write_data5[] = {
	0xff, 0x09, 0x5a, 0x00, 0x03, 0x00, 0x01, 0x40, 0x22, 0x4e, 0xe0, 0xfe, 0x87, 0xd5, 0xb4, 0xf2,
	0x5f, 0x17, 0x0e, 0xbf, 0x6e, 0xcd, 0xea, 0xaf, 0x3f, 0x84, 0x2a, 0xe7, 0x10, 0x9c, 0x9e, 0x2d,
	0xf7, 0x66, 0x4e, 0xf5, 0x59, 0xbe, 0x7f, 0xc9, 0x8f, 0x46, 0x0d, 0x60, 0xbd, 0x32, 0x2a, 0x83,
	0xbd, 0xb2, 0xcf, 0x3c, 0xbb, 0x71, 0x25, 0xc3, 0x45, 0xe0, 0x49, 0x09, 0x48, 0x1a, 0x48, 0xd2,
	0xd9, 0xba, 0xcb, 0x6a, 0x50, 0xdd, 0xdf, 0xb2, 0x10, 0x71, 0x6e, 0x2c, 0xd9, 0x7e, 0x05, 0x51,
	0xc1, 0x1c, 0xa3, 0xd1, 0x1a, 0x10, 0x49, 0x5a, 0xaf, 0x31,
};




// static uint8_t write_data_1[] = {
// 	0xff, 0x09, 0x3d, 0x00, 0x03, 0x00, 0x01, 0x00, 
// 	0x03, 0xa1, 0x04, 0x1e, 0xd0, 0x68, 0x67, 0xa2,
// 	0x24, 0x64, 0x35, 0x38, 0x66, 0x36, 0x38, 0x30,
// 	0x65, 0x2d, 0x32, 0x39, 0x34, 0x37, 0x2d, 0x34,
// 	0x33, 0x32, 0x39, 0x2d, 0x62, 0x39, 0x63, 0x62,
// 	0x2d, 0x63, 0x39, 0x35, 0x32, 0x30, 0x30, 0x35,
// 	0x34, 0x36, 0x34, 0x61, 0x35, 0xa3, 0x01, 0x20,
// 	0xa4, 0x02, 0x00, 0xf0, 0xfc
// };


static bool adv_data_parse_cb(struct bt_data *data, void *user_data)
{
        char *name = user_data;
        uint8_t len;

        switch (data->type) {
        case BT_DATA_NAME_SHORTENED:
        case BT_DATA_NAME_COMPLETE:
                len = MIN(data->data_len, ADV_NAME_STR_MAX_LEN - 1);
                memcpy(name, data->data, len);
                name[len] = '\0';
                return false;
        default:
                return true;
        }
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %s\n",
		addr, connectable ? "yes" : "no");
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_ERR("scan_connecting_error");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	LOG_INF("scan_connecting");
	default_conn = bt_conn_ref(conn);
}

static void scan_filter_no_match(struct bt_scan_device_info *device_info,
				 bool connectable)
{
	// int err;
	// struct bt_conn *conn = NULL;
	char addr[BT_ADDR_LE_STR_LEN];
	char name_str[ADV_NAME_STR_MAX_LEN] = {0};

	// if (device_info->recv_info->adv_type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
	// 	bt_addr_le_to_str(device_info->recv_info->addr, addr,
	// 			  sizeof(addr));
	// 	printk("Direct advertising received from %s\n", addr);
	// 	bt_scan_stop();

	// 	err = bt_conn_le_create(device_info->recv_info->addr,
	// 				BT_CONN_LE_CREATE_CONN,
	// 				device_info->conn_param, &conn);

	// 	if (!err) {
	// 		default_conn = bt_conn_ref(conn);
	// 		bt_conn_unref(conn);
	// 	}
	// }

	bt_addr_le_to_str(device_info->recv_info->addr, addr,
				  sizeof(addr));

	bt_data_parse(device_info->adv_data, adv_data_parse_cb, name_str);

	if (strlen(name_str) > 0) {
		LOG_INF("no_match: %s", name_str);
	}
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
		scan_connecting_error, scan_connecting);

static void scan_init(void)
{
	int ret;

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	// err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_BAS);
	// if (err) {
	// 	printk("Scanning filters cannot be set (err %d)\n", err);

	// 	return;
	// }

	// err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	// if (err) {
	// 	printk("Filters cannot be turned on (err %d)\n", err);
	// }

	ret = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, "Anker SOLIX F2000");
	if (ret) {
		printk("Scanning filters cannot be set (ret %d)\n", ret);

		return;
	}

	ret = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
	if (ret) {
		printk("Filters cannot be turned on (ret %d)\n", ret);
	}
}

static uint8_t gatt_read_cb(struct bt_conn *conn, uint8_t err,
                            struct bt_gatt_read_params *params,
                            const void *data, uint16_t length)
{
	LOG_INF("gatt_read_cb");

        if (err != BT_ATT_ERR_SUCCESS) {
                LOG_ERR("Read failed: 0x%02X\n", err);
        }

        // if (params->single.handle == chrc_handle) {
        //         if (length != CHRC_SIZE ||
        //             memcmp(data, chrc_data, length) != 0) {
        //                 FAIL("chrc data different than expected", err);
        //         }
        // } else if (params->single.handle == chrc_handle) {
        //         if (length != LONG_CHRC_SIZE ||
        //             memcmp(data, long_chrc_data, length) != 0) {
        //                 FAIL("long_chrc data different than expected", err);
        //         }
        // }

        // (void)memset(params, 0, sizeof(*params));

        LOG_HEXDUMP_INF(data, length, "bt data:");

        return 0;
}

/**
 * Internal function to process report notification and pass it further.
 *
 * @param conn   Connection handler.
 * @param params Notification parameters structure - the pointer
 *               to the structure provided to subscribe function.
 * @param data   Pointer to the data buffer.
 * @param length The size of the received data.
 *
 * @retval BT_GATT_ITER_STOP     Stop notification
 * @retval BT_GATT_ITER_CONTINUE Continue notification
 */
static uint8_t notify_process(struct bt_conn *conn,
                           struct bt_gatt_subscribe_params *params,
                           const void *data, uint16_t length)
{
	LOG_INF("notify_process");

        // struct bt_bnotify_processs_client *bas;
        // uint8_t battery_level;
        // const uint8_t *bdata = data;

        // bas = CONTAINER_OF(params, struct bt_bas_client, notify_params);
        // if (!data || !length) {
        //         LOG_INF("Notifications disabled.");
        //         if (bas->notify_cb) {
        //                 bas->notify_cb(bas, BT_BAS_VAL_INVALID);
        //         }
        //         return BT_GATT_ITER_STOP;
        // }
        // if (length != 1) {
        //         LOG_ERR("Unexpected notification value size.");
        //         if (bas->notify_cb) {
        //                 bas->notify_cb(bas, BT_BAS_VAL_INVALID);
        //         }
        //         return BT_GATT_ITER_STOP;
        // }

        // battery_level = bdata[0];
        // if (battery_level > BT_BAS_VAL_MAX) {
        //         LOG_ERR("Unexpected notification value.");
        //         if (bas->notify_cb) {
        //                 bas->notify_cb(bas, BT_BAS_VAL_INVALID);
        //         }
        //         return BT_GATT_ITER_STOP;
        // }
        // bas->battery_level = battery_level;
        // if (bas->notify_cb) {
        //         bas->notify_cb(bas, battery_level);
        // }

	LOG_HEXDUMP_INF(data, length, "notify data:");

        return BT_GATT_ITER_CONTINUE;
}


static uint16_t handle;
static bool subscribed;
static bool disconnect_sent;


static void write_callback(struct bt_conn *conn, uint8_t err,
			   struct bt_gatt_write_params *params)
{
        LOG_DBG("write_callback: err=%d", err);
}


static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
        if (!err) {
                LOG_INF("MTU exchange done");
        } else {
                LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
        }
}


int main(void)
{
	const struct device *wdt = DEVICE_DT_GET(DT_NODELABEL(wdt0));
#if defined(CONFIG_APP_SUSPEND_CONSOLE)
	const struct device *cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
#endif
	int ret;
	uint32_t reset_cause;
	int main_wdt_chan_id = -1;
	uint32_t events;

	ret = watchdog_new_channel(wdt, &main_wdt_chan_id);
	if (ret < 0) {
		LOG_ERR("Could allocate main watchdog channel");
		return ret;
	}

	// ret = watchdog_start(wdt);
	// if (ret < 0) {
	// 	LOG_ERR("Could allocate start watchdog");
	// 	return ret;
	// }

	LOG_INF("\n\n🚀 MAIN START (%s) 🚀\n", APP_VERSION_FULL);

	reset_cause = show_reset_cause();
	clear_reset_cause();
	
	if (app_event_manager_init()) {
		LOG_ERR("Event manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}

	LOG_INF("🆗 initialized");

#if defined(CONFIG_APP_SUSPEND_CONSOLE)
	ret = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	if (ret < 0) {
		LOG_ERR("Could not suspend the console");
		return ret;
	}
#endif




	ret = bt_enable(NULL);
	if (ret) {
		printk("Bluetooth init failed (ret %d)\n", ret);
		return 0;
	}

	scan_init();

	ret = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (ret) {
		printk("Scanning failed to start (ret %d)\n", ret);
		return 0;
	}

	LOG_INF("Scanning successfully started");


	k_sleep(K_SECONDS(5));


	bt_scan_stop();


	thread_analyzer_print(0);

	LOG_INF("┌──────────────────────────────────────────────────────────┐");
	LOG_INF("│ Entering main loop                                       │");
	LOG_INF("└──────────────────────────────────────────────────────────┘");

	while (1) {
		LOG_INF("💤 waiting for events");
		events = k_event_wait(&button_events,
				(BUTTON_PRESS_EVENT),
				true,
				K_SECONDS(CONFIG_APP_MAIN_LOOP_PERIOD_SEC));

		LOG_INF("⏰ events: %08x", events);


		// if (!subscribed && handle != 0) {
		if (!subscribed) {
			k_sleep(K_SECONDS(5));

			LOG_DBG("Trying to subscribe");

			subscribed = true;

			subscribe_params.notify = notify_process;
			subscribe_params.value = BT_GATT_CCC_NOTIFY;
			// subscribe_params.value = BT_GATT_CCC_INDICATE;
			subscribe_params.value_handle = 0x0e;
			subscribe_params.ccc_handle = 0x0f;
			atomic_set_bit(subscribe_params.flags,
			       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

			ret = bt_gatt_subscribe(default_conn, &subscribe_params);
			if (ret) {
				LOG_ERR("Report notification subscribe error: %d.", ret);
			}
			LOG_DBG("Subscription sent");


			k_sleep(K_SECONDS(2));

			// 0000   ff 09 36 00 03 00 01 00 01 a1 04 36 12 68 67 a2
			// 0010   24 64 35 38 66 36 38 30 65 2d 32 39 34 37 2d 34
			// 0020   33 32 39 2d 62 39 63 62 2d 63 39 35 32 30 30 35
			// 0030   34 36 34 61 35 cb

			// P9
			// 0}ÀL\eP=9Rÿ	6¡6hg¢$d58f680e-2947-4329-b9cb-c952005464a5Ë~ 1


		        // exchange_params.func = exchange_func;
		        // ret = bt_gatt_exchange_mtu(default_conn, &exchange_params);
		        // if (ret) {
		        //         LOG_WRN("MTU exchange failed (err %d)", ret);
		        // }

		        // k_sleep(K_SECONDS(2));

			write_params.func = write_callback;
		        write_params.handle = 0x0c;
		        write_params.offset = 0;
		        write_params.data = write_data1;
		        write_params.length = ARRAY_SIZE(write_data1);

		        ret = bt_gatt_write(default_conn, &write_params);
		        if (ret) {
				LOG_ERR("Could not write: %d.", ret);
			}
			LOG_DBG("Write sent");

			k_sleep(K_SECONDS(1));

			// write_params.func = write_callback;
		        // write_params.handle = 0x0c;
		        // write_params.offset = 0;
		        write_params.data = write_data2;
		        write_params.length = ARRAY_SIZE(write_data2);

		        ret = bt_gatt_write(default_conn, &write_params);
		        if (ret) {
				LOG_ERR("Could not write: %d.", ret);
			}
			LOG_DBG("Write sent");

			k_sleep(K_SECONDS(1));

			// write_params.func = write_callback;
		        // write_params.handle = 0x0c;
		        // write_params.offset = 0;
		        write_params.data = write_data3;
		        write_params.length = ARRAY_SIZE(write_data3);

		        ret = bt_gatt_write(default_conn, &write_params);
		        if (ret) {
				LOG_ERR("Could not write: %d.", ret);
			}
			LOG_DBG("Write sent");

			k_sleep(K_SECONDS(1));

			// write_params.func = write_callback;
		        // write_params.handle = 0x0c;
		        // write_params.offset = 0;
		        write_params.data = write_data4;
		        write_params.length = ARRAY_SIZE(write_data4);

		        ret = bt_gatt_write(default_conn, &write_params);
		        if (ret) {
				LOG_ERR("Could not write: %d.", ret);
			}
			LOG_DBG("Write sent");

			k_sleep(K_SECONDS(1));

			// write_params.func = write_callback;
		        // write_params.handle = 0x0c;
		        // write_params.offset = 0;
		        write_params.data = write_data5;
		        write_params.length = ARRAY_SIZE(write_data5);

		        ret = bt_gatt_write(default_conn, &write_params);
		        if (ret) {
				LOG_ERR("Could not write: %d.", ret);
			}
			LOG_DBG("Write sent");

		}

		if (!disconnect_sent && k_uptime_get() > 60000) {
			ret = bt_conn_disconnect(default_conn,
				BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			if (ret) {
				LOG_ERR("Could not disconnect: %d.", ret);
			}
			else {
				LOG_DBG("⛓️‍💥 Disconnect sent");
			}

			disconnect_sent = true;
		}

		if (events & BUTTON_PRESS_EVENT) {
			LOG_INF("handling button press event");
		}

		LOG_INF("🦴 feed watchdog");
		wdt_feed(wdt, main_wdt_chan_id);
	}

	return 0;
}

static bool event_handler(const struct app_event_header *eh)
{
	const struct button_event *evt;

	if (is_button_event(eh)) {
		evt = cast_button_event(eh);

		if (evt->pressed) {
			LOG_INF("🛎️  Button pressed");
			k_event_post(&button_events, BUTTON_PRESS_EVENT);
		}
	}

	return true;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, button_event);
