#include <bits/stdint-uintn.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "./ccs881.c"
#include "CCS811.h"
#include <linux/i2c-dev.h>

#include "./types.h"
#include "./prom.c"

#ifdef _DEBUG_
#define DEBUG_PRINTF(X...) printf(X)
#else
#define DEBUG_PRINTF(X...)
#endif

#define print_bits(x)                                                                              \
	do {                                                                                           \
		unsigned long long a__ = (x);                                                              \
		size_t			   bits__ = sizeof(x) * 8;                                                 \
		while (bits__--)                                                                           \
			putchar(a__ &(1ULL << bits__) ? '1' : '0');                                            \
	} while (0)

// Global variables for easier clean-ups.

int device_fd = 0;
int sock_fd = 0;
int looping = 1;
int data_wait = 1;

// READ_MODE defines the way the sensor will aggregate data.
// Possible modes are:
//
//  - CCS811_MODE_IDLE
//  - CCS811_MODE_1SEC
//  - CCS811_MODE_10SEC
//  - CCS811_MODE_60SEC
//  - CCS811_MODE_250MS
const uint8_t READ_MODE = (CCS811_MODE_10SEC << 4);

/// Populate the passed buffer with the current time as an RFC3339 string.
static char *gettime(char *buf) {
	// Preumption is that the buffer is able to hold a string
	// of type "YYYY-MM-DDTHH:mm:ss.000Z" (24 chars) and a
	// termination character.
	memset(buf, 0, 25);

	time_t now;
	time(&now);
	strftime(buf, 25, "%FT%T.000Z", gmtime(&now));

	return buf;
}

void handle_sigint(int signal) {
	DEBUG_PRINTF("Caught SIGINT\n");
	looping = 0;
	data_wait = 0;
}

int main() {
	// Stop any running loops on SIGINT.
	signal(SIGINT, handle_sigint);

	// cmd is a convinience variable for sending commands to the sensor.
	uint8_t cmd;
	// status_out is a buffer for the result of CCS811_STATUS command.
	uint8_t status_out;
	// ok handles read/write error codes.
	int ok;
	// data is the output buffer for CCS811_ALG_RESULT_DATA calls.
	ccs811_measurement_t data;
	// Set sane defaults.
	data.eco2 = 400;
	data.tvoc = 0;
	// timebuf is a buffer for pretty-printing timestamps.
	char timebuf[80] = {0};

	http_thread_args http_args;
	http_args.running = &looping;
	http_args.sensor_data = &data;
	http_args.sock_fd = &sock_fd;
	http_args.address = "0.0.0.0";
	http_args.port = 2112;

	pthread_t http_thread;
    pthread_create(&http_thread, NULL, listen_and_serve, (void *)(&http_args));
    // pthread_join(http_thread, NULL);

	// Try to acquire I2C for read/writes.
	{
		const char *filename = "/dev/i2c-0";
		DEBUG_PRINTF("Opening '%s' for read/writes ... ", filename);
		device_fd = open(filename, O_RDWR);
		if (device_fd < 0) {
			printf("Error with open()!: %s\n", strerror(errno));
			exit(1);
		}
		DEBUG_PRINTF("OK\n");
	}

	// Set communication address
	const int addr = CCS811_SLAVEADDR_0;
	if (ioctl(device_fd, I2C_SLAVE, addr) < 0) {
		printf("Error with ioctl()!: %s\n", strerror(errno));
		goto abort_close;
	}

	// Reset the device
	{
		DEBUG_PRINTF("Resetting the sensor ... ");
		uint8_t const cmd_sw_reset[5] = {CCS811_SW_RESET, 0x11, 0xE5, 0x72, 0x8A};
		if ((ok = write(device_fd, cmd_sw_reset, 5)) != 5) {
			printf("Resetting the sensor failed: %s (write: %d)\n", strerror(errno), ok);
			goto abort_close;
		}
		DEBUG_PRINTF("OK\n");
		usleep(CCS811_WAIT_AFTER_RESET_US);
	}

	// Check that the reset was successful
	{
		DEBUG_PRINTF("Reading status_out ... ");
		cmd = CCS811_STATUS;
		if ((ok = write(device_fd, &cmd, 1)) != 1) {
			printf("cannot request read: %s (write: %d)\n", strerror(errno), ok);
			goto abort_close;
		}
		if (read(device_fd, &status_out, 1) != 1) {
			printf("cannot read: %s\n", strerror(errno));
			goto abort_close;
		}
		DEBUG_PRINTF("OK\n");
	}

	// Run some basic checks
	{
		DEBUG_PRINTF("Status is: 0x%x\n", status_out);
		if (status_out != 0x10) {
			printf("ccs811: Not in boot mode, or no valid app\n");
			goto abort_close;
		}

		const int hwid = hardware_id(device_fd);
		const int hwv = hardware_version(device_fd);
		const int bl = bootloader_version(device_fd);
		const int app = application_version(device_fd);

		if (hwid != 0x81) {
			printf("Uknown hardware id: 0x%x", hwid);
			goto abort_close;
		}

		printf("hardware ID:        \t0x%x\t(%d)\n", hwid, hwid);
		printf("hardware version:   \t0x%x\t(%d)\n", hwv, hwv);
		printf("bootloader version: \t0x%x\t(%d)\n", bl, bl);
		printf("application version:\t0x%x\t(%d)\n", app, app);

		cmd = CCS811_STATUS;
		write(device_fd, &cmd, 1);
		read(device_fd, &status_out, 1);
		if ((status_out & 0b00010000) == 0) {
			printf("no valid firmware\n");
			goto abort_close;
		}
	}

	// Enable Application mode
	{
		DEBUG_PRINTF("Starting sensor in app mode ...");
		cmd = CCS811_APP_START;
		if (write(device_fd, &cmd, 1) != 1) {
			printf("cannot enable app mode: %s\n", strerror(errno));
			goto abort_close;
		}
		DEBUG_PRINTF("OK\n");
		usleep(CCS811_WAIT_AFTER_APPSTART_US);

		cmd = CCS811_STATUS;
		write(device_fd, &cmd, 1);
		read(device_fd, &status_out, 1);
		if ((status_out & 0b10000000) == 0) {
			printf("still in firmware mode\n");
			goto abort_close;
		}
	}

	// Set data collection period
	{
		DEBUG_PRINTF("Setting measurement mode ... ");

		uint8_t const meas_cmd[2] = {CCS811_MEAS_MODE, READ_MODE};
		if (write(device_fd, meas_cmd, 2) != 2) {
			printf("cannot set measurement mode: %s\n", strerror(errno));
			goto abort_close;
		}
		DEBUG_PRINTF("OK\n");

		// Verify that it's set
		cmd = CCS811_MEAS_MODE;
		if (write(device_fd, &cmd, 1) != 1) {
			printf("cannot request measurement mode: %s\n", strerror(errno));
			goto abort_close;
		}
		if (read(device_fd, &status_out, 1) != 1) {
			printf("cannot read: %s\n", strerror(errno));
			goto abort_close;
		}

		DEBUG_PRINTF("Measurement mode is: ");
#ifdef _DEBUG_
		print_bits(status_out);
		printf("\n");
#endif
	}

	printf("Check for errors ... ");
	cmd = CCS811_ERROR_ID;
	if (write(device_fd, &cmd, 1) != 1) {
		printf("failed reading error: %s\n", strerror(errno));
		goto abort_close;
	}
	uint8_t err = 0;
	if (read(device_fd, &err, 1) != 1) {
		printf("cannot read: %s\n", strerror(errno));
		goto abort_close;
	}
	if (err != 0) {
		printf("Error raised: ");
		print_bits(err);
		printf("\n");
		goto abort_close;
	} else {
		printf("OK\n");
	}

	DEBUG_PRINTF("Starting data loop\n");
	while (looping) {

		// Always wait for data :)
		DEBUG_PRINTF("[%s] Waiting for data\n", gettime(timebuf));
		while (data_wait) {
			cmd = CCS811_STATUS;
			if ((ok = write(device_fd, &cmd, 1)) != 1) {
				printf("cannot request read: %s (write: %d)\n", strerror(errno), ok);
				goto abort_close;
			}
			if (read(device_fd, &status_out, 1) != 1) {
				printf("cannot read: %s\n", strerror(errno));
				goto abort_close;
			}
			if (status_out & 0b0001000) {
				data_wait = 0;
				break;
			}

			// Although there is a mode of the sensor which reports data
			// each 250ms I will not be using it - Poll each second.
			sleep(1);
		}
		data_wait = 1;

		// Read the data
		DEBUG_PRINTF("[%s] New data present\n", gettime(timebuf));
		{
			DEBUG_PRINTF("Trying to read algorithm data ... ");
			memset(&data, 0, sizeof(ccs811_measurement_t));

			cmd = CCS811_ALG_RESULT_DATA;
			if (write(device_fd, &cmd, 1) != 1) {
				printf("failed requesting algorithm data: %s\n", strerror(errno));
				goto abort_close;
			}
			if (read(device_fd, &data, 8) < 5) {
				printf("failed reading algorithm data: %s\n", strerror(errno));
				goto abort_close;
			}
			DEBUG_PRINTF("OK\n");
			// The sensor is a big-endian-based processor.
			data.eco2 = be16toh(data.eco2);
			data.tvoc = be16toh(data.tvoc);

			if (!(data.status & 0x90)) {
				printf("ccs811: Not in app mode, or no valid app: ");
				print_bits(data.status);
				goto abort_close;
			}

			gettime(timebuf);
			// printf(
			// 	"{\"co2\":%d,\"tvoc\":%d,\"error\":%d,\"timestamp\":\"%s\"}\n",
			// 	data.eco2,
			// 	data.tvoc,
			// 	data.error_id,
			// 	timebuf
			// );
			printf("%d,%d,%d,%s\n", data.eco2, data.tvoc, data.error_id, timebuf); // CSV
			fflush(stdout);
			DEBUG_PRINTF("Reported error: 0x%x\n", data.error_id);
			DEBUG_PRINTF("eCO2 readings:  %huppm\n", data.eco2);
			DEBUG_PRINTF("eTVOC readings: %huppb\n", data.tvoc);
		}
	}

	// Cleanup
	close(device_fd);
	close(sock_fd);
	return EXIT_SUCCESS;

abort_close:
	close(device_fd);
	close(sock_fd);
	exit(errno);
}
