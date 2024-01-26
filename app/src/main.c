#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/debug/thread_analyzer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <app_version.h>

#include "reset.h"
#include "watchdog.h"


int main(void)
{
	const struct device *wdt = DEVICE_DT_GET(DT_NODELABEL(wdt0));
#if defined(CONFIG_APP_SUSPEND_CONSOLE)
	const struct device *cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
#endif
	uint32_t reset_cause;
	int main_wdt_chan_id = -1;


	watchdog_init(wdt, &main_wdt_chan_id);

	LOG_INF("\n\n🚀 MAIN START (%s) 🚀\n", APP_VERSION_FULL);

	reset_cause = show_reset_cause();
	clear_reset_cause();

	LOG_INF("🎉 init done 🎉");

#if defined(CONFIG_APP_SUSPEND_CONSOLE)
	pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
#endif

	thread_analyzer_print();

	while (1) {
		LOG_INF("🦴 feed watchdog");
		wdt_feed(wdt, main_wdt_chan_id);

		LOG_INF("💤 sleeping until next loop");
		k_sleep(K_SECONDS(CONFIG_APP_MAIN_LOOP_PERIOD_SEC));
	}

	return 0;
}
