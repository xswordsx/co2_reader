/**
 * @file CCS811.h
 * @brief Various constants for working with the CCS811 sensor.
 *
 * Most of the constansts were taken from:
 * * https://github.com/sciosense/CCS811_driver/blob/97abe9b45b5793f030f39bd06f0d07f166800377/src/ScioSense_CCS811.cpp
 * * https://github.com/sciosense/CCS811_driver/blob/97abe9b45b5793f030f39bd06f0d07f166800377/src/ScioSense_CCS811.h
 *
 * @author ScioSense (https://github.com/sciosense)
 * @bug No known bugs.
 */

// clang-format off

#ifndef _CCS881_CONSTS_
#define _CCS881_CONSTS_

// Version of this CCS811 driver
#define CCS811_VERSION 0x12

// I2C slave address for ADDR 0 respectively 1
#define CCS811_SLAVEADDR_0 0x5A
#define CCS811_SLAVEADDR_1 0x5B

// The values for mode in ccs811_start()
#define CCS811_MODE_IDLE  0b000
#define CCS811_MODE_1SEC  0b001
#define CCS811_MODE_10SEC 0b010
#define CCS811_MODE_60SEC 0b011
#define CCS811_MODE_250MS 0b100

// The flags for errstat in ccs811_read()
// ERRSTAT is a merge of two hardware registers: ERROR_ID (bits 15-8) and STATUS
// (bits 7-0) Also bit 1 (which is always 0 in hardware) is set to 1 when an I2C
// read error occurs
#define CCS811_ERRSTAT_ERROR             0x0001 ///< There is an error, the ERROR_ID register (0xE0) contains the error source
#define CCS811_ERRSTAT_I2CFAIL           0x0002 ///< Bit flag added by software: I2C transaction error
#define CCS811_ERRSTAT_DATA_READY        0x0008 ///< A new data sample is ready in ALG_RESULT_DATA
#define CCS811_ERRSTAT_APP_VALID         0x0010 ///< Valid application firmware loaded
#define CCS811_ERRSTAT_FW_MODE           0x0080 ///< Firmware is in application mode (not boot mode)
#define CCS811_ERRSTAT_WRITE_REG_INVALID 0x0100 ///< Received an I²C write request addressed to this station but with invalid register address ID
#define CCS811_ERRSTAT_READ_REG_INVALID  0x0200 ///< The CCS811 received an I²C read request to a mailbox ID that is invalid
#define CCS811_ERRSTAT_MEASMODE_INVALID  0x0400 ///< The CCS811 received an I²C request to write an unsupported mode to MEAS_MODE
#define CCS811_ERRSTAT_MAX_RESISTANCE    0x0800 ///< The sensor resistance measurement has reached or exceeded the maximum range
#define CCS811_ERRSTAT_HEATER_FAULT      0x1000 ///< The heater current in the CCS811 is not in range
#define CCS811_ERRSTAT_HEATER_SUPPLY     0x2000 ///< The heater voltage is not being applied correctly

// These flags should not be set. They flag errors.
#define CCS811_ERRSTAT_HWERRORS (CCS811_ERRSTAT_ERROR | CCS811_ERRSTAT_WRITE_REG_INVALID | CCS811_ERRSTAT_READ_REG_INVALID | CCS811_ERRSTAT_MEASMODE_INVALID | CCS811_ERRSTAT_MAX_RESISTANCE | CCS811_ERRSTAT_HEATER_FAULT | CCS811_ERRSTAT_HEATER_SUPPLY)
#define CCS811_ERRSTAT_ERRORS (CCS811_ERRSTAT_I2CFAIL | CCS811_ERRSTAT_HWERRORS)
// These flags should normally be set - after a measurement. They flag data
// available (and valid app running).
#define CCS811_ERRSTAT_OK (CCS811_ERRSTAT_DATA_READY | CCS811_ERRSTAT_APP_VALID | CCS811_ERRSTAT_FW_MODE)
// These flags could be set after a measurement. They flag data is not yet
// available (and valid app running).
#define CCS811_ERRSTAT_OK_NODATA (CCS811_ERRSTAT_APP_VALID | CCS811_ERRSTAT_FW_MODE)

// Timings
#define CCS811_WAIT_AFTER_RESET_US     2000 // The CCS811 needs a wait after reset
#define CCS811_WAIT_AFTER_APPSTART_US  1000 // The CCS811 needs a wait after app start
#define CCS811_WAIT_AFTER_WAKE_US      50   // The CCS811 needs a wait after WAKE signal
#define CCS811_WAIT_AFTER_APPERASE_MS  500  // The CCS811 needs a wait after app erase (300ms from spec not enough)
#define CCS811_WAIT_AFTER_APPVERIFY_MS 70   // The CCS811 needs a wait after app verify
#define CCS811_WAIT_AFTER_APPDATA_MS   50   // The CCS811 needs a wait after writing app data

// Main interface
// =====================================================================================================

// CCS811 registers/mailboxes, all 1 byte except when stated otherwise
#define CCS811_STATUS          0x00
#define CCS811_MEAS_MODE       0x01
#define CCS811_ALG_RESULT_DATA 0x02 /// up to 8 bytes
#define CCS811_RAW_DATA        0x03 /// 2 bytes
#define CCS811_ENV_DATA        0x05 /// 4 bytes
#define CCS811_THRESHOLDS      0x10 /// 5 bytes
#define CCS811_BASELINE        0x11 /// 2 bytes
#define CCS811_HW_ID           0x20
#define CCS811_HW_VERSION      0x21
#define CCS811_FW_BOOT_VERSION 0x23 /// 2 bytes
#define CCS811_FW_APP_VERSION  0x24 /// 2 bytes
#define CCS811_ERROR_ID        0xE0
#define CCS811_APP_ERASE       0xF1 /// 4 bytes
#define CCS811_APP_DATA        0xF2 /// 9 bytes
#define CCS811_APP_VERIFY      0xF3 /// 0 bytes
#define CCS811_APP_START       0xF4 /// 0 bytes
#define CCS811_SW_RESET        0xFF /// 4 bytes

#endif