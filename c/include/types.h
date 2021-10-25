#include <bits/stdint-uintn.h>

#ifndef __CO2_READER_TYPES__
#define __CO2_READER_TYPES__

/// This structure is a copy of the memory layout of the
/// response from CCS881's ALG_RESULT_DATA call.
/// NOTICE: Since the sensor uses big-endians the data
/// will need re-ordering after read.
typedef struct ccs811_measurement_t {
	uint16_t eco2;
	uint16_t tvoc;
	uint8_t	 status;
	uint8_t	 error_id;
	uint16_t raw_data;
} ccs811_measurement_t;

typedef struct http_thread_args {
	int* running;
    int* sock_fd;

    char *address;
    uint16_t port;
    int backlog;

    struct ccs811_measurement_t* sensor_data;
} http_thread_args;

#endif // __CO2_READER_TYPES__