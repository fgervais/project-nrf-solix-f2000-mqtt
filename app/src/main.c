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

// static struct bt_gatt_read_params read_params;
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;


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

static uint8_t discovery_callback(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       struct bt_gatt_discover_params *params)
{
	char uuid_str[BT_UUID_STR_LEN];
	int err;

	if (!attr) {
		LOG_DBG("NULL attribute");
	} else {
		LOG_DBG("Attr: handle %u", attr->handle);

		handle = attr->handle;

		// LOG_DBG("Trying to subscribe");

		// subscribe_params.notify = notify_process;
		// subscribe_params.value = BT_GATT_CCC_NOTIFY;
		// subscribe_params.value_handle = attr->handle;
		// subscribe_params.ccc_handle = 0;
		// atomic_set_bit(subscribe_params.flags,
		//        BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

		// err = bt_gatt_subscribe(default_conn, &subscribe_params);
		// if (err) {
		// 	LOG_ERR("Report notification subscribe error: %d.", err);
		// }
		// LOG_DBG("Report subscribed.");
	}

	bt_uuid_to_str(attr->uuid, uuid_str, sizeof(uuid_str));
	LOG_DBG("UUID: %s", uuid_str);

	// if (conn != bt_gatt_dm_inst.conn) {
	// 	LOG_ERR("Unexpected conn object. Aborting.");
	// 	discovery_complete_error(&bt_gatt_dm_inst, -EFAULT);
	// 	return BT_GATT_ITER_STOP;
	// }

	// switch (params->type) {
	// case BT_GATT_DISCOVER_PRIMARY:
	// case BT_GATT_DISCOVER_SECONDARY:
	// 	return discovery_process_service(&bt_gatt_dm_inst,
	// 					 attr, params);
	// case BT_GATT_DISCOVER_ATTRIBUTE:
	// 	return discovery_process_attribute(&bt_gatt_dm_inst,
	// 					   attr, params);
	// case BT_GATT_DISCOVER_CHARACTERISTIC:
	// 	return discovery_process_characteristic(&bt_gatt_dm_inst,
	// 						attr,
	// 						params);
	// default:
	// 	/* This should not be possible */
	// 	__ASSERT(false, "Unknown param type.");
	// 	discovery_complete_error(&bt_gatt_dm_inst, -EINVAL);

	// 	break;
	// }


	return BT_GATT_ITER_STOP;
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


	if (default_conn != NULL) {
		LOG_INF("We are connected, trying to read");

		// read_params.func = gatt_read_cb;
		// // read_params.handle_count = 0;
		// read_params.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
        	// read_params.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		// read_params.by_uuid.uuid = service_uuid;

		// ret = bt_gatt_read(default_conn, &read_params);
		// if (ret != 0) {
		// 	LOG_ERR("bt_gatt_read failed: %d", ret);
		// }



		discover_params.func = discovery_callback;
		discover_params.uuid = service_uuid;
                discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
                discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
                // discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
                discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

                ret = bt_gatt_discover(default_conn, &discover_params);



		// bas->notify_params.notify = notify_process;
		// bas->notify_params.value = BT_GATT_CCC_NOTIFY;
		// bas->notify_params.value_handle = bas->val_handle;
		// bas->notify_params.ccc_handle = bas->ccc_handle;
		// atomic_set_bit(bas->notify_params.flags,
		// 	       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
	}



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


		if (!subscribed && handle != 0) {
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
			LOG_DBG("Report subscribed.");


		}


		k_sleep(K_SECONDS(5));

		// 0000   ff 09 36 00 03 00 01 00 01 a1 04 36 12 68 67 a2
		// 0010   24 64 35 38 66 36 38 30 65 2d 32 39 34 37 2d 34
		// 0020   33 32 39 2d 62 39 63 62 2d 63 39 35 32 30 30 35
		// 0030   34 36 34 61 35 cb

		// P9
		// 0}ÀL\eP=9Rÿ	6¡6hg¢$d58f680e-2947-4329-b9cb-c952005464a5Ë~ 1

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
