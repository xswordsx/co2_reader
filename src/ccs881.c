/**
 * @brief I2C helpers for the CCS811 module.
 *
 * @author Ivan Latunov
 */

#ifndef _I2C_CCS881_HELPERS_
#define _I2C_CCS881_HELPERS_

#include "CCS811.h"
#include <bits/stdint-uintn.h>

// clang-format off

int hardware_id(int fd) {
	uint8_t out = CCS811_HW_ID;
	if (write(fd, &out, 1) != 1) return -1;
	if (read(fd, &out, 1) != 1) return -1;
	return out;
}

int hardware_version(int fd) {
	uint8_t out = CCS811_HW_VERSION;
	if (write(fd, &out, 1) != 1) return -1;
	if (read(fd, &out, 1) != 1) return -1;
	return out;
}

// Gets version of the CCS811 boot loader (returns -1 on I2C failure)
int bootloader_version(int fd) {
	uint8_t buf[2] = { CCS811_FW_BOOT_VERSION, 0 };
	if (write(fd, buf, 1) != 1) return -1;
	if (read(fd, buf, 2) != 2) return -1;
	return 256*buf[0] + buf[1];
}

// Gets version of the CCS811 application (returns -1 on I2C failure)
int application_version(int fd) {
	uint8_t buf[2] = { CCS811_FW_APP_VERSION, 0 };
	if (write(fd, buf, 1) != 1) return -1;
	if (read(fd, buf, 2) != 2) return -1;
	return 256*buf[0] + buf[1];
}

int sensor_status(int fd) {
	uint8_t buf[1] = {CCS811_STATUS};
	if (write(fd, buf, 1) != 1) return -1;
	if (read(fd, buf, 1) != 1) return -1;
	return buf[0];
}

#endif