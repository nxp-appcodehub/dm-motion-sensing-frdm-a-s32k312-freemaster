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
#include "motion_detector.h"
#include "Siul2_Port_Ip.h"
#include <math.h>
#include "OsIf.h"
#include "freemaster.h"

/*==================================================================================================
 *                                      DEFINES AND MACROS
 ==================================================================================================*/
/* Shake detection: minimum total delta acceleration to trigger (m/s²) */
#define SHAKE_THRESHOLD     20.0f

/* Shake animation: number of LED flash cycles */
#define SHAKE_FLASH_COUNT   3U

/* Shake cooldown: iterations to skip tilt updates after shake */
#define SHAKE_COOLDOWN_INIT 10U

/**
 * @brief  Number of consecutive high-delta samples required to confirm a shake.
 *
 * At 50 ms sample period:
 *   - Real shake (~4 Hz): each half-cycle ≈ 125 ms → produces high deltas
 *     on every sample → counter reaches 3 in 150 ms (passes easily).
 *   - Table slide: impulse + 1–2 ringing samples from Click connector,
 *     then settles → counter never reaches 3 before a calm sample resets it.
 */
#define SHAKE_CONSECUTIVE_REQUIRED  3U

/*==================================================================================================
 *                                      LOCAL VARIABLES
 ==================================================================================================*/
/* Countdown timer to prevent immediate re-triggering after shake */
static uint32 shake_cooldown = 0U;

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief Blocking millisecond delay using OsIf system counter
 *
 * Uses hardware counter for accurate timing. Should only be used
 * during initialization; use non-blocking timers in the main loop.
 *
 * @param delay  Delay duration in milliseconds
 */
void Delay_ms(uint32 delay) {
    uint32 cur = OsIf_GetCounter(OSIF_COUNTER_SYSTEM);
    uint32 elapsed = 0U;
    uint32 timeout = OsIf_MicrosToTicks(delay * 1000U, OSIF_COUNTER_SYSTEM);

    while (elapsed < timeout) {
        elapsed += OsIf_GetElapsed(&cur, OSIF_COUNTER_SYSTEM);
    }
}

/**
 * @brief Turn off all RGB LEDs
 */
void TurnOffAllLEDS(void) {
    Siul2_Dio_Ip_WritePin(RED_LED_PORT, RED_LED_PIN, 0U);
    Siul2_Dio_Ip_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, 0U);
    Siul2_Dio_Ip_WritePin(BLUE_LED_PORT, BLUE_LED_PIN, 0U);
}

/**
 * @brief Turn on all RGB LEDs (white)
 */
void TurnOnAllLEDS(void) {
    Siul2_Dio_Ip_WritePin(RED_LED_PORT, RED_LED_PIN, 1U);
    Siul2_Dio_Ip_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, 1U);
    Siul2_Dio_Ip_WritePin(BLUE_LED_PORT, BLUE_LED_PIN, 1U);
}

/**
 * @brief Set individual RGB LED states
 *
 * @param red    RED LED state (0 = off, 1 = on)
 * @param green  GREEN LED state (0 = off, 1 = on)
 * @param blue   BLUE LED state (0 = off, 1 = on)
 */
void SetLEDColor(uint8 red, uint8 green, uint8 blue) {
    Siul2_Dio_Ip_WritePin(RED_LED_PORT, RED_LED_PIN, red);
    Siul2_Dio_Ip_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, green);
    Siul2_Dio_Ip_WritePin(BLUE_LED_PORT, BLUE_LED_PIN, blue);
}

/**
 * @brief Detect shake gesture by comparing current and previous acceleration
 *
 * Calculates the sum of absolute deltas across all three axes.
 * A shake is detected when the total delta exceeds SHAKE_THRESHOLD.
 *
 * @param x  Current X-axis acceleration (m/s²)
 * @param y  Current Y-axis acceleration (m/s²)
 * @param z  Current Z-axis acceleration (m/s²)
 * @return 1 if shake detected, 0 otherwise
 */
uint8 DetectShake(float x, float y, float z) {
    static float prev_x = 0.0f, prev_y = 0.0f, prev_z = 0.0f;
    static uint8 first_run = 1U;
    static uint8 consecutive_count = 0U;

    /* Skip first run to initialize previous values */
    if (first_run) {
        prev_x = x;
        prev_y = y;
        prev_z = z;
        first_run = 0U;
        return 0U;
    }

    /* Calculate change in acceleration on each axis */
    float delta_x = fabsf(x - prev_x);
    float delta_y = fabsf(y - prev_y);
    float delta_z = fabsf(z - prev_z);
    float total_delta = delta_x + delta_y + delta_z;

    /* Always update previous values for next iteration */
    prev_x = x;
    prev_y = y;
    prev_z = z;

    /* Debounce: require consecutive high-delta samples */
    if (total_delta > SHAKE_THRESHOLD) {
        consecutive_count++;
        if (consecutive_count >= SHAKE_CONSECUTIVE_REQUIRED) {
            consecutive_count = 0U;
            return 1U;  /* Confirmed shake */
        }
    } else {
        /* Any calm sample resets the counter */
        consecutive_count = 0U;
    }

    return 0U;
}

/**
 * @brief Blocking shake animation with 3 quick LED flash cycles
 *
 * WARNING: This is a blocking function. Use non-blocking shake
 * animation in main loop for production code.
 */
void HandleShakeAnimation(void) {
    uint8 i;

    for (i = 0U; i < SHAKE_FLASH_COUNT; i++) {
        TurnOnAllLEDS();
        Delay_ms(1000U);
        TurnOffAllLEDS();
        Delay_ms(1000U);
    }

    /* Set cooldown to prevent re-triggering immediately */
    shake_cooldown = SHAKE_COOLDOWN_INIT;
}

/**
 * @brief Update RGB LED color based on board tilt direction
 *
 * Lights a specific color depending on which axis has dominant
 * acceleration above AXIS_THRESHOLD, and whether the value
 * is positive or negative:
 *   +X → Red,    -X → Orange (Red+Green)
 *   +Y → Green,  -Y → Cyan   (Green+Blue)
 *   +Z → Blue,   -Z → Magenta (Red+Blue)
 *
 * @param x  X-axis acceleration (m/s²)
 * @param y  Y-axis acceleration (m/s²)
 * @param z  Z-axis acceleration (m/s²)
 */
void UpdateTiltIndicator(float x, float y, float z) {
    float accelX_abs = fabsf(x);
    float accelY_abs = fabsf(y);
    float accelZ_abs = fabsf(z);

    /* Turn off all LEDs first */
    TurnOffAllLEDS();

    /* Light LED based on dominant axis */
    if (accelX_abs > AXIS_THRESHOLD) {
        if (accelY_abs < AXIS_THRESHOLD && accelZ_abs < AXIS_THRESHOLD) {
            /* X-axis dominant */
            if (x > 0) {
                SetLEDColor(1, 0, 0);   /* Positive X: RED */
            } else {
                SetLEDColor(1, 1, 0);   /* Negative X: ORANGE */
            }
        }
    }
    else if (accelY_abs > AXIS_THRESHOLD) {
        if (accelX_abs < AXIS_THRESHOLD && accelZ_abs < AXIS_THRESHOLD) {
            /* Y-axis dominant */
            if (y > 0) {
                SetLEDColor(0, 1, 0);   /* Positive Y: GREEN */
            } else {
                SetLEDColor(0, 1, 1);   /* Negative Y: CYAN */
            }
        }
    }
    else if (accelZ_abs > AXIS_THRESHOLD) {
        if (accelY_abs < AXIS_THRESHOLD && accelX_abs < AXIS_THRESHOLD) {
            /* Z-axis dominant */
            if (z > 0) {
                SetLEDColor(0, 0, 1);   /* Positive Z: BLUE */
            } else {
                SetLEDColor(1, 0, 1);   /* Negative Z: MAGENTA */
            }
        }
    }
}

/**
 * @brief Get current shake cooldown counter value
 * @return Remaining cooldown iterations before tilt resumes
 */
uint32 GetShakeCooldown(void) {
    return shake_cooldown;
}

/**
 * @brief Decrement shake cooldown counter by one (if > 0)
 */
void DecrementShakeCooldown(void) {
    if (shake_cooldown > 0U) {
        shake_cooldown--;
    }
}
