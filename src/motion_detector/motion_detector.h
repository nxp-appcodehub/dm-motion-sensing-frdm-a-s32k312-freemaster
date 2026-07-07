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

#ifndef MOTION_DETECTOR_H
#define MOTION_DETECTOR_H

/*==================================================================================================
 *                                        INCLUDE FILES
 ==================================================================================================*/
#include "Mcal.h"
#include "Siul2_Dio_Ip.h"
#include "Siul2_Port_Ip_Cfg.h"

/*==================================================================================================
 *                                      DEFINES AND MACROS
 ==================================================================================================*/
/* Tilt detection threshold in m/s² */
#define AXIS_THRESHOLD 6U

/*==================================================================================================
 *                                   FUNCTION PROTOTYPES
 ==================================================================================================*/

/**
 * @brief Blocking delay using OsIf system counter
 * @param delay  Delay duration in milliseconds
 */
void Delay_ms(uint32 delay);

/**
 * @brief Turn off all RGB LEDs (RED, GREEN, BLUE)
 */
void TurnOffAllLEDS(void);

/**
 * @brief Turn on all RGB LEDs (RED, GREEN, BLUE)
 */
void TurnOnAllLEDS(void);

/**
 * @brief Set individual RGB LED states
 * @param red    RED LED state (0 = off, 1 = on)
 * @param green  GREEN LED state (0 = off, 1 = on)
 * @param blue   BLUE LED state (0 = off, 1 = on)
 */
void SetLEDColor(uint8 red, uint8 green, uint8 blue);

/**
 * @brief Detect shake gesture from acceleration delta
 * @param x  X-axis acceleration in m/s²
 * @param y  Y-axis acceleration in m/s²
 * @param z  Z-axis acceleration in m/s²
 * @return 1 if shake detected, 0 otherwise
 */
uint8 DetectShake(float x, float y, float z);

/**
 * @brief Blocking shake animation (3 quick LED flashes)
 */
void HandleShakeAnimation(void);

/**
 * @brief Update RGB LED color based on board tilt direction
 * @param x  X-axis acceleration in m/s²
 * @param y  Y-axis acceleration in m/s²
 * @param z  Z-axis acceleration in m/s²
 */
void UpdateTiltIndicator(float x, float y, float z);

/**
 * @brief Get current shake cooldown counter
 * @return Remaining cooldown iterations
 */
uint32 GetShakeCooldown(void);

/**
 * @brief Decrement shake cooldown counter by one
 */
void DecrementShakeCooldown(void);

#endif /* MOTION_DETECTOR_H */
