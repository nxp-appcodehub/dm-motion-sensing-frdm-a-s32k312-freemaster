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

#ifndef FXLS8964AF_H
#define FXLS8964AF_H

/*==================================================================================================
 *                                        INCLUDE FILES
 ==================================================================================================*/
#include "Lpi2c_Ip.h"
#include "Siul2_Port_Ip.h"

/*==================================================================================================
 *                                      DEFINES AND MACROS
 ==================================================================================================*/
/* I2C Configuration */
#define ACCEL_ADDR          0x18U       /* FXLS8964 I2C slave address */
#define LPI2C_INSTANCE      1U          /* LPI2C peripheral instance */
#define TIMEOUT             1000000U    /* I2C transfer timeout value */

/* FXLS8964 Register Addresses */
#define FXLS8964_WHO_AM_I       0x13U   /* Device identification register */
#define FXLS8964_OUT_X_LSB      0x04U   /* X-axis output LSB register */
#define FXLS8964_SENS_CONFIG1   0x15U   /* Sensor configuration register 1 */
#define FXLS8964_SENS_CONFIG2   0x16U   /* Sensor configuration register 2 */
#define FXLS8964_SENS_CONFIG3   0x17U   /* Sensor configuration register 3 */

/*==================================================================================================
 *                                   FUNCTION PROTOTYPES
 ==================================================================================================*/

/**
 * @brief Initialize the FXLS8964 accelerometer sensor
 * @return LPI2C status indicating success or failure
 */
Lpi2c_Ip_StatusType InitAccelerometer(void);

/**
 * @brief Read raw acceleration data from FXLS8964 (6 bytes: X, Y, Z)
 * @param accelData  Pointer to buffer for raw accelerometer data
 * @return LPI2C status indicating success or failure
 */
Lpi2c_Ip_StatusType ReadAccelerometer(uint8 *accelData);

/**
 * @brief Convert raw FXLS8964 data to acceleration in m/s²
 * @param accelData  Pointer to raw accelerometer data (6 bytes)
 * @param x          Pointer to store X-axis acceleration (m/s²)
 * @param y          Pointer to store Y-axis acceleration (m/s²)
 * @param z          Pointer to store Z-axis acceleration (m/s²)
 */
void ConvertAcceleration_FXLS8964(uint8 *accelData, float *x, float *y, float *z);

#endif /* FXLS8964AF_H */
