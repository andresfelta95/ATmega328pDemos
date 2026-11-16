/*
 * GCC_I2C.c
 *
 * Created: 2023-04-01 9:05:13 AM
 * Author : Andres
 * Description: This is a demo of the I2C library for the ATmega328P
 *              It will use a 128x32 OLED display to display the
 *              the reading of the VL53L1CX distance sensor.
 *
 *  VL53L1CX specifications:

        - Emitter: 940 nm invisible laser (Class1)
        - SPAD (single photon avalanche diode) receiving array with integrated lens
        - FlightSense™ ranging sensor for fast and accurate long distance ranging
        - Up to 400 cm distance measurement
        - Up to 50 Hz ranging frequency
        - Typical full field-of-view (FoV): 27 °
        - Programmable region-of-interest (ROI) size on the receiving array, allowing the sensor FoV to be reduced
        - Programmable ROI position on the receiving array, providing multizone operation control from the host
        - Can be hidden behind many cover window materials
        - I²C interface (up to 400 kHz)
        - Shutdown and interrupt pins
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "I2C.h"
#include "SSD1306.h"
#include "VL53L1X_api.h"

//  The ST API identifies the sensor with a 'dev' handle; this port has one sensor
#define TOF_DEV 0

//  Global Variables
uint16_t _Distance = 0;         // last distance, in mm
uint8_t _DataReady = 0;         // set by the sensor when a new measurement is ready
uint8_t _RangeStatus = 0;       // 0 = valid measurement, anything else = error/warning
uint8_t _BootState = 0;         // non-zero once the sensor firmware has booted
//  Create a 128byte buffer for the I2C scan
unsigned char I2CScanResults[128];
// One display row: 21 characters + terminator
char _Line[22];

int main(void)
{
    //  Initialize the I2C bus
    I2C_Init(F_CPU, I2CBus100);
    //  Scan the I2C bus (results are useful when debugging the wiring)
    I2C_Scan(I2CScanResults);
    //  Initialize the SSD1306 OLED display (also clears it)
    SSD1306_DispInit();
    SSD1306_DisplayOn();

    //  Wait for the VL53L1X to boot
    while (_BootState == 0)
    {
        VL53L1X_BootState(TOF_DEV, &_BootState);
        _delay_ms(2);
    }

    //  Initialize the VL53L1X sensor
    VL53L1X_ERROR status = VL53L1X_SensorInit(TOF_DEV);
    status |= VL53L1X_SetDistanceMode(TOF_DEV, 1);          // 1 = short (up to ~1.3 m)
    status |= VL53L1X_SetTimingBudgetInMs(TOF_DEV, 500);
    status |= VL53L1X_SetInterMeasurementInMs(TOF_DEV, 500);
    //  Start continuous ranging measurements
    status |= VL53L1X_StartRanging(TOF_DEV);

    if (status)
    {
        SSD1306_StringXY(0, 1, "VL53L1X init error");
        SSD1306_Render();
        while (1)
            ;
    }

    while (1)
    {
        //  Wait for a new measurement
        _DataReady = 0;
        while (_DataReady == 0)
        {
            VL53L1X_CheckForDataReady(TOF_DEV, &_DataReady);
        }
        VL53L1X_GetRangeStatus(TOF_DEV, &_RangeStatus);
        VL53L1X_GetDistance(TOF_DEV, &_Distance);
        VL53L1X_ClearInterrupt(TOF_DEV);

        //  Show the distance as 12.3 cm. Every line is padded to the full
        //  21-character row so it overwrites the previous text without
        //  clearing (and flickering) the whole display.
        if (_RangeStatus == 0)
            snprintf(_Line, sizeof _Line, "Distance: %3u.%u cm  ", _Distance / 10, _Distance % 10);
        else
            snprintf(_Line, sizeof _Line, "No valid range (%3u) ", _RangeStatus);
        SSD1306_StringXY(0, 3, _Line);
        SSD1306_Render();

        _delay_ms(500);
    }
}
