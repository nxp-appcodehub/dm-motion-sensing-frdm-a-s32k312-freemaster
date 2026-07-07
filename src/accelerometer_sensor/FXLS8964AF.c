/*==================================================================================================
* Project : RTD AUTOSAR 4.9
* Platform : CORTEXM
* Peripheral : S32K3XX
* Dependencies : none
*
* Autosar Version : 4.9.0
* Autosar Revision : ASR_REL_4_9_REV_0000
* Autosar Conf.Variant :
* SW Version : 7.0.0
* Build Version : S32K3_RTD_7_0_0_QLP03_D2512_ASR_REL_4_9_REV_0000_20251210
*
* Copyright 2020 - 2026 NXP
*
*   NXP Proprietary. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms. By expressly
*   accepting such terms or by downloading, installing, activating and/or otherwise
*   using the software, you are agreeing that you have read, and that you agree to
*   comply with and are bound by, such license terms. If you do not agree to be
*   bound by the applicable license terms, then you may not retain, install,
*   activate or otherwise use the software.
==================================================================================================*/

/*==================================================================================================
 *                                        INCLUDE FILES
 ==================================================================================================*/
#include <FXLS8964AF.h>
#include "motion_detector.h"
#include "Lpi2c_Ip.h"

/*==================================================================================================
 *                                      STATIC VARIABLES
 ==================================================================================================*/
/** Sensor sensitivity in g/LSB */
static float g_sensitivity = 0.00094f;  /* default: 0.98 mg/LSB */

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief Initialize the FXLS8964 accelerometer via I2C
 *
 * Configures the sensor in standby mode, sets data format,
 * output data rate (100 Hz), and activates the sensor.
 *
 * @return LPI2C_IP_SUCCESS_STATUS on success, error status otherwise
 */
Lpi2c_Ip_StatusType InitAccelerometer(void) {
    Lpi2c_Ip_StatusType status;
    uint8 writeData[2];
    uint8 whoAmIReg = FXLS8964_WHO_AM_I;
    uint8 whoAmIValue = 0;

    /* Set slave address */
    Lpi2c_Ip_MasterSetSlaveAddr(LPI2C_INSTANCE, ACCEL_ADDR, (boolean) FALSE);

	/* Step 1: Verify communication with WHO_AM_I register */
	status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, &whoAmIReg, 1U,
			(boolean) FALSE, TIMEOUT);
	if (status == LPI2C_IP_SUCCESS_STATUS) {
		status = Lpi2c_Ip_MasterReceiveDataBlocking(LPI2C_INSTANCE,
				&whoAmIValue, 1U, (boolean) TRUE, TIMEOUT);

		/* Check if it's FXLS8964 (0x86) or FXLS8974 (0x87) */
		if (whoAmIValue != 0x86U && whoAmIValue != 0x87U) {
			return LPI2C_IP_ERROR_STATUS;
		}

	} else {
		return status;
	}

    Delay_ms(100U);

    /* Step 2: Enter Standby mode (required for configuration) */
    writeData[0] = FXLS8964_SENS_CONFIG1;
    writeData[1] = 0x00U;  /* ACTIVE=0, FSR=00b (±2g) */
    status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, writeData, 2U,
            (boolean) TRUE, TIMEOUT);
    if (status != LPI2C_IP_SUCCESS_STATUS)
        return status;

    Delay_ms(100U);

    /* Step 3: Configure data format (little-endian, 12-bit) */
    writeData[0] = FXLS8964_SENS_CONFIG2;
    writeData[1] = 0x00U;  /* LE_BE=0, F_READ=0, LPM */
    status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, writeData, 2U,
            (boolean) TRUE, TIMEOUT);
    if (status != LPI2C_IP_SUCCESS_STATUS)
        return status;

    Delay_ms(100U);

    /* Step 4: Set output data rate to 100 Hz */
    writeData[0] = FXLS8964_SENS_CONFIG3;
    writeData[1] = 0x55U;  /* WAKE_ODR=100Hz, SLEEP_ODR=100Hz */
    status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, writeData, 2U,
            (boolean) TRUE, TIMEOUT);
    if (status != LPI2C_IP_SUCCESS_STATUS)
        return status;

    Delay_ms(100U);

    /* Step 5: Enter Active mode */
    writeData[0] = FXLS8964_SENS_CONFIG1;
    writeData[1] = 0x01U;  /* ACTIVE=1, FSR=00b (±2g) */
    status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, writeData, 2U,
            (boolean) TRUE, TIMEOUT);

    /* Wait for sensor to stabilize */
    Delay_ms(500U);

    return status;
}

/**
 * @brief Read 6 bytes of raw acceleration data from FXLS8964
 *
 * Reads X, Y, Z axis data starting from OUT_X_LSB register (0x04).
 * Each axis provides 2 bytes (LSB + MSB) in little-endian format.
 *
 * @param accelData  Pointer to buffer (minimum 6 bytes)
 * @return LPI2C_IP_SUCCESS_STATUS on success, error status otherwise
 */
Lpi2c_Ip_StatusType ReadAccelerometer(uint8 *accelData) {
    Lpi2c_Ip_StatusType status;
    uint8 regAddr = FXLS8964_OUT_X_LSB;

    /* Set slave address */
    Lpi2c_Ip_MasterSetSlaveAddr(LPI2C_INSTANCE, ACCEL_ADDR, (boolean) FALSE);

    /* Send register address (no STOP, repeated start) */
    status = Lpi2c_Ip_MasterSendDataBlocking(
            LPI2C_INSTANCE, &regAddr, 1U, (boolean) FALSE, TIMEOUT);

    if (status == LPI2C_IP_SUCCESS_STATUS) {
        /* Read 6 bytes: X_LSB, X_MSB, Y_LSB, Y_MSB, Z_LSB, Z_MSB */
        status = Lpi2c_Ip_MasterReceiveDataBlocking(
                LPI2C_INSTANCE, accelData, 6U, (boolean) TRUE, TIMEOUT);
    }
    return status;
}

/**
 * @brief Convert raw FXLS8964 data to acceleration in m/s²
 *
 * Raw data is 12-bit sign-extended 2's complement in little-endian format.
 * Sensitivity at ±2g range: 1 LSB = 1/1024 g.
 *
 * @param accelData  Raw data buffer (6 bytes)
 * @param x          Output X-axis acceleration (m/s²)
 * @param y          Output Y-axis acceleration (m/s²)
 * @param z          Output Z-axis acceleration (m/s²)
 */
void ConvertAcceleration_FXLS8964(uint8 *accelData, float *x, float *y, float *z) {
    /* Combine LSB and MSB into 16-bit signed values (little-endian) */
    int16_t rawX = (int16_t) ((accelData[1] << 8) | accelData[0]);
    int16_t rawY = (int16_t) ((accelData[3] << 8) | accelData[2]);
    int16_t rawZ = (int16_t) ((accelData[5] << 8) | accelData[4]);

    /* Sign-extend 12-bit values to 16-bit */
    if (rawX & 0x0800) rawX |= 0xF000;
    if (rawY & 0x0800) rawY |= 0xF000;
    if (rawZ & 0x0800) rawZ |= 0xF000;

    /* Convert to m/s² using detected sensor sensitivity */
    const float G_TO_MS2 = 9.80665f;

    *x = (float)rawX * g_sensitivity * G_TO_MS2;
    *y = (float)rawY * g_sensitivity * G_TO_MS2;
    *z = (float)rawZ * g_sensitivity * G_TO_MS2;
}
