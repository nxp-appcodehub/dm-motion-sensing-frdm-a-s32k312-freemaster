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

#ifdef __cplusplus
extern "C"{
#endif

/*==================================================================================================
 *                                        INCLUDE FILES
 ==================================================================================================*/
#include "S32K312.h"
#include "Mcal.h"
#include "Clock_Ip.h"
#include "Lpi2c_Ip.h"
#include "Lpi2c_Ip_Cfg.h"
#include "Siul2_Port_Ip_Cfg.h"

#include <FXLS8964AF.h>
#include "motion_detector.h"
#include "OsIf.h"

#include "IntCtrl_Ip.h"
#include "Lpuart_Uart_Ip.h"
#include "freemaster.h"
#include "freemaster_s32_lpuart.h"

/*==================================================================================================
 *                                      DEFINES AND MACROS
 ==================================================================================================*/
/* Sensor sampling period in milliseconds */
#define SENSOR_SAMPLE_PERIOD_MS     50U

/* Shake animation flash period in milliseconds */
#define SHAKE_FLASH_PERIOD_MS       300U

/* Shake animation total flash count */
#define SHAKE_FLASH_TOTAL           3U

/*==================================================================================================
 *                                      GLOBAL VARIABLES
 ==================================================================================================*/
/* System exit code */
volatile int exit_code = 0;

/* Raw sensor data buffers */
uint8 accelData[6];

/* Processed sensor values (volatile for FreeMASTER visibility) */
volatile float accelX = 0.0f;
volatile float accelY = 0.0f;
volatile float accelZ = 0.0f;

/*==================================================================================================
 *                                      LOCAL VARIABLES
 ==================================================================================================*/
/* Sensor sampling timer state */
static uint32 timer_start = 0U;
static uint32 timer_elapsed = 0U;
static uint32 timer_timeout = 0U;

/* Shake animation timer state */
static uint32  shake_timer_start   = 0U;
static uint32  shake_timer_elapsed = 0U;
static uint32  shake_timer_timeout = 0U;
static uint8   shake_flash_count   = 0U;
static boolean shake_led_on        = TRUE;
static boolean shake_animating     = FALSE;

/*==================================================================================================
 *                                   LOCAL FUNCTION PROTOTYPES
 ==================================================================================================*/
static void Timer_Start(uint32 period_ms);
static boolean Timer_IsExpired(void);
static void ShakeTimer_Start(uint32 period_ms);
static boolean ShakeTimer_IsExpired(void);
static void ReadAndUpdateAccelData(void);

/*==================================================================================================
 *                                   EXTERNAL FUNCTION PROTOTYPES
 ==================================================================================================*/
extern void FMSTR_LPUART6_Isr(void);

/*==================================================================================================
 *                                           MAIN FUNCTION
 ==================================================================================================*/
int main(void)
{
    Lpi2c_Ip_StatusType i2cStatus;

    /*==============================================================================================
     *                                 SYSTEM INITIALIZATION
     ==============================================================================================*/
    /* Clock initialization */
    Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);
    OsIf_Init(NULL_PTR);

    /* GPIO pin initialization */
    Siul2_Port_Ip_Init(
        NUM_OF_CONFIGURED_PINS_PortContainer_0_BOARD_InitPeripherals,
        g_pin_mux_InitConfigArr_PortContainer_0_BOARD_InitPeripherals);

    /* LPUART6 initialization for FreeMASTER communication */
    Lpuart_Uart_Ip_Init(LPUART_UART_IP_INSTANCE_USING_6,
                        &Lpuart_Uart_Ip_xHwConfigPB_6);

    /* Interrupt controller initialization (LPUART6 ISR for FreeMASTER) */
    IntCtrl_Ip_Init(&IntCtrlConfig_0);

    /* FreeMASTER serial communication initialization */
    FMSTR_SerialSetBaseAddress((FMSTR_ADDR)IP_LPUART_6_BASE);
    FMSTR_Init();

    /* LPI2C1 master initialization for sensor communication */
    Lpi2c_Ip_MasterInit(LPI2C_INSTANCE, &I2c_Lpi2cMaster_HwChannel1_Channel0);

    /* Turn off all LEDs at startup */
    TurnOffAllLEDS();

	i2cStatus = InitAccelerometer();
	if (i2cStatus != LPI2C_IP_SUCCESS_STATUS) {
		exit_code = 1;
	}

    /* Start sensor sampling timer */
    Timer_Start(SENSOR_SAMPLE_PERIOD_MS);

    /*==============================================================================================
     *                                 MAIN CONTROL LOOP
     ==============================================================================================*/
    for (;;)
    {
        if (exit_code != 0) break;

        /* ================================================================ */
        /*  SHAKE ANIMATION STATE                                           */
        /* ================================================================ */
        if (shake_animating)
        {
            /* Handle shake LED toggle */
            if (ShakeTimer_IsExpired())
            {
                if (shake_led_on)
                {
                    TurnOffAllLEDS();
                    shake_led_on = FALSE;
                    ShakeTimer_Start(SHAKE_FLASH_PERIOD_MS);
                }
                else
                {
                    shake_flash_count++;
                    if (shake_flash_count >= SHAKE_FLASH_TOTAL)
                    {
                        /* Animation complete */
                        shake_animating = FALSE;
                        shake_flash_count = 0U;
                        TurnOffAllLEDS();
                    }
                    else
                    {
                        TurnOnAllLEDS();
                        shake_led_on = TRUE;
                        ShakeTimer_Start(SHAKE_FLASH_PERIOD_MS);
                    }
                }
            }

            /* Keep reading accelerometer for FreeMASTER graphs */
            if (Timer_IsExpired())
            {
                Timer_Start(SENSOR_SAMPLE_PERIOD_MS);
                ReadAndUpdateAccelData();
            }
            continue;
        }

        /* ================================================================ */
        /*  NORMAL STATE: sensor read + LED control                         */
        /* ================================================================ */
        if (!Timer_IsExpired())
        {
            continue;   /* Timer not expired, fast loop */
        }

        /* Timer expired — read sensors */
        Timer_Start(SENSOR_SAMPLE_PERIOD_MS);

        /* --- Accelerometer reading --- */
        i2cStatus = ReadAccelerometer(accelData);
        if (i2cStatus == LPI2C_IP_SUCCESS_STATUS)
        {
            float tmpX, tmpY, tmpZ;
            ConvertAcceleration_FXLS8964(accelData, &tmpX, &tmpY, &tmpZ);

            /* Update volatile globals for FreeMASTER */
            accelX = tmpX;
            accelY = tmpY;
            accelZ = tmpZ;

			uint8 shakeState = DetectShake(tmpX, tmpY, tmpZ);
			if (shakeState) {
				/* Enter shake animation state */
				shake_animating = TRUE;
				shake_flash_count = 0U;
				shake_led_on = TRUE;
				TurnOnAllLEDS();
				ShakeTimer_Start(SHAKE_FLASH_PERIOD_MS);
			} else if (GetShakeCooldown() > 0U) {
				DecrementShakeCooldown();
				TurnOffAllLEDS();
            }
            else
            {
                /* Normal tilt indicator */
                UpdateTiltIndicator(tmpX, tmpY, tmpZ);
            }
        }
    }

    return exit_code;
}

/*==================================================================================================
 *                                       LOCAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief LPUART6 ISR handler for FreeMASTER serial communication
 */
void FMSTR_LPUART6_Isr(void)
{
    FMSTR_SerialIsr();
}

/**
 * @brief Start the sensor sampling non-blocking timer
 * @param period_ms  Timer period in milliseconds
 */
static void Timer_Start(uint32 period_ms)
{
    timer_start   = OsIf_GetCounter(OSIF_COUNTER_SYSTEM);
    timer_elapsed = 0U;
    timer_timeout = OsIf_MicrosToTicks(period_ms * 1000U, OSIF_COUNTER_SYSTEM);
}

/**
 * @brief Check if sensor sampling timer has expired
 * @return TRUE if expired, FALSE otherwise
 */
static boolean Timer_IsExpired(void)
{
    timer_elapsed += OsIf_GetElapsed(&timer_start, OSIF_COUNTER_SYSTEM);
    return (timer_elapsed >= timer_timeout) ? TRUE : FALSE;
}

/**
 * @brief Start the shake animation flash non-blocking timer
 * @param period_ms  Flash half-period in milliseconds
 */
static void ShakeTimer_Start(uint32 period_ms)
{
    shake_timer_start   = OsIf_GetCounter(OSIF_COUNTER_SYSTEM);
    shake_timer_elapsed = 0U;
    shake_timer_timeout = OsIf_MicrosToTicks(period_ms * 1000U, OSIF_COUNTER_SYSTEM);
}

/**
 * @brief Check if shake animation flash timer has expired
 * @return TRUE if expired, FALSE otherwise
 */
static boolean ShakeTimer_IsExpired(void)
{
    shake_timer_elapsed += OsIf_GetElapsed(&shake_timer_start, OSIF_COUNTER_SYSTEM);
    return (shake_timer_elapsed >= shake_timer_timeout) ? TRUE : FALSE;
}

/**
 * @brief Read accelerometer and update global volatile variables
 *
 * Reads raw data from FXLS8964, converts to m/s², and stores
 * in accelX/Y/Z for FreeMASTER graph visualization.
 * Does NOT control LEDs — caller decides LED behavior.
 */
static void ReadAndUpdateAccelData(void)
{
    Lpi2c_Ip_StatusType i2cStatus;

    i2cStatus = ReadAccelerometer(accelData);
    if (i2cStatus == LPI2C_IP_SUCCESS_STATUS)
    {
        float tmpX, tmpY, tmpZ;
        ConvertAcceleration_FXLS8964(accelData, &tmpX, &tmpY, &tmpZ);

        accelX = tmpX;
        accelY = tmpY;
        accelZ = tmpZ;
    }
}

#ifdef __cplusplus
}
#endif
