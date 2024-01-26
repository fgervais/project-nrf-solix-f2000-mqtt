#ifndef WATCHDOG_H_
#define WATCHDOG_H_

int watchdog_init(const struct device *wdt,
		  int *main_channel_id);

#endif /* WATCHDOG_H_ */