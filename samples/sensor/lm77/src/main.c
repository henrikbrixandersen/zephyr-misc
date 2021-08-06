/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <shell/shell.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

struct sensor_trigger trigger = {
	.chan = SENSOR_CHAN_AMBIENT_TEMP,
	.type = SENSOR_TRIG_THRESHOLD,
};

static void trigger_handler(const struct device *dev, struct sensor_trigger *trigger)
{
	struct sensor_value val;
	int err;

	err = sensor_sample_fetch(dev);
	if (err) {
		LOG_ERR("failed to fetch triggered sample (err %d)", err);
		return;
	}

	err = sensor_channel_get(dev, trigger->chan, &val);
	if (err) {
		LOG_ERR("failed to get triggered channel (err %d)", err);
		return;
	}

	printf("Triggered, Temp = %10.6f\n", sensor_value_to_double(&val));
}

static int cmd_trigger(const struct shell *shell, size_t argc, char *argv[])
{
	sensor_trigger_handler_t handler;
	const struct device *dev;
	uint32_t value;
	int err;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(shell, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	value = strtoul(argv[2], NULL, 0);
	if (value) {
		handler = trigger_handler;
	} else {
		handler = NULL;
	}

	err = sensor_trigger_set(dev, &trigger, handler);
	if (err) {
		LOG_ERR("failed to set trigger (err %d)", err);
		return err;
	}

	return 0;
}

static int attr_helper(const struct shell *shell, size_t argc, char *argv[],
			   const char *name, enum sensor_attribute attr)
{
	const struct device *dev;
	struct sensor_value val;
	int32_t millideg;
	int err;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(shell, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	if (argc > 2) {
		millideg = strtoul(argv[2], NULL, 0);
		sensor_value_from_double(&val, millideg / 1000.0);

		err = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP, attr, &val);
		if (err) {
			shell_error(shell, "Failed to get attribute (err %d)", err);
			return err;
		}
	} else {
		err = sensor_attr_get(dev, SENSOR_CHAN_AMBIENT_TEMP, attr, &val);
		if (err) {
			shell_error(shell, "Failed to set attribute (err %d)", err);
			return err;
		}

		shell_print(shell, "%s = %10.6f", name, sensor_value_to_double(&val));
	}

	return 0;
}

static int cmd_tlow(const struct shell *shell, size_t argc, char *argv[])
{

	return attr_helper(shell, argc, argv, "T_low", SENSOR_ATTR_LOWER_THRESH);
}

static int cmd_thigh(const struct shell *shell, size_t argc, char *argv[])
{
	return attr_helper(shell, argc, argv, "T_high", SENSOR_ATTR_UPPER_THRESH);
}

static int cmd_tcrit(const struct shell *shell, size_t argc, char *argv[])
{
	return attr_helper(shell, argc, argv, "T_crit", SENSOR_ATTR_ALERT);
}

static int cmd_thyst(const struct shell *shell, size_t argc, char *argv[])
{
	return attr_helper(shell, argc, argv, "T_hyst", SENSOR_ATTR_HYSTERESIS);
}

static int cmd_shutdown(const struct shell *shell, size_t argc, char *argv[])
{
	const struct device *dev;
	enum pm_device_state state;
	uint32_t value;
	int err;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(shell, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	if (argc > 2) {
		value = strtoul(argv[2], NULL, 0);
		if (value) {
			state = PM_DEVICE_STATE_SUSPENDED;
		} else {
			state = PM_DEVICE_STATE_ACTIVE;
		}

		err = pm_device_state_set(dev, state);
		if (err) {
			shell_error(shell, "Failed to set power state (err %d)", err);
			return err;
		}
	} else {
		err = pm_device_state_get(dev, &state);
		if (err) {
			shell_error(shell, "Failed to get power state (err %d)", err);
			return err;
		}

		value = state == PM_DEVICE_STATE_ACTIVE ? 0 : 1;
		shell_print(shell, "shutdown = %d", value);
	}

	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = NULL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_lm77,
	SHELL_CMD_ARG(tlow, &dsub_device_name, "tlow", cmd_tlow, 2, 1),
	SHELL_CMD_ARG(thigh, &dsub_device_name, "thigh", cmd_thigh, 2, 1),
	SHELL_CMD_ARG(tcrit, &dsub_device_name, "tcrit", cmd_tcrit, 2, 1),
	SHELL_CMD_ARG(thyst, &dsub_device_name, "thyst", cmd_thyst, 2, 1),
	SHELL_CMD_ARG(shutdown, &dsub_device_name, "shutdown", cmd_shutdown, 2, 1),
	SHELL_CMD_ARG(trigger, &dsub_device_name, "trigger", cmd_trigger, 3, 0),
	SHELL_SUBCMD_SET_END
	);

SHELL_CMD_REGISTER(lm77, &sub_lm77, "LM77 sensor commands", NULL);
