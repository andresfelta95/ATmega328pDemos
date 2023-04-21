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

#define F_CPU 16E6

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "I2C.h"
#include "SSD1306.h"
#include "VL53L1X_api.h"
#include "VL53L1X_calibration.h"

//  Global Variables
uint16_t _Distance = 0;
uint16_t xtalk = 0;
uint16_t _Xtalk = 0;
int16_t offSet = 0;
int16_t _Offset = 0;
VL53L1X_ERROR status = 0;
uint8_t state = 0;
uint8_t _DataReady = 0;
uint8_t _RangeStatus = 0;
//  Create a 128byte buffer for the I2C scan
unsigned char I2CScanResults[128];
// String to show the distance as 10.00 cm
char distance[10];
char xtalkString1[20];
char xtalkString2[20];
char offsetString1[20];
char offsetString2[20];



int main(void)
{
    //  Initialize the I2C bus
    I2C_Init(F_CPU, I2CBus100);
    //  Scan the I2C bus
    I2C_Scan(I2CScanResults);
    // //  Initialize the SSD1306 OLED display
    SSD1306_DispInit();
    // //  Turn on the display
    SSD1306_DisplayOn();
    // //  Clear the display
    SSD1306_Clear();
    SSD1306_Render();
    // //  Writhe my name on the display
    // SSD1306_StringXY(0, 2, "Andres");
    // SSD1306_StringXY(7, 2, "Tangarife");
    // SSD1306_Render();
    while(state == 0)
    {
        status = VL53L1X_BootState(0, &state);
        _delay_ms(2);
    }
    //  Initialize the VL53L1X sensor
    status = VL53L1X_SensorInit(0);
    //  Calibrate the sensor
    status = VL53L1X_CalibrateOffset(0, 140, &offSet);
    status = VL53L1X_GetOffset(0, &_Offset);
    status = VL53L1X_SetOffset(0, _Offset);
    status = VL53L1X_CalibrateXtalk(0, 140, &xtalk);
    status = VL53L1X_GetXtalk(0, &_Xtalk);
    status = VL53L1X_SetXtalk(0, _Xtalk);

    // Set the 2 offset values in a string
    sprintf(offsetString1, "Offset: %d", offSet);
    sprintf(offsetString2, "Offset: %d", _Offset);
    // Set the 2 xtalk values in a string
    sprintf(xtalkString1, "Xtalk: %u", xtalk);
    sprintf(xtalkString2, "Xtalk: %u", _Xtalk);
    // Display the offset and xtalk values on the display
    SSD1306_StringXY(0, 0, offsetString1);
    SSD1306_StringXY(0, 1, offsetString2);
    SSD1306_StringXY(0, 2, xtalkString1);
    // SSD1306_StringXY(0, 3, xtalkString2);
    SSD1306_Render();

    status = VL53L1X_SetDistanceMode(0, 1);
    status = VL53L1X_SetTimingBudgetInMs(0, 20);
    status = VL53L1X_SetInterMeasurementInMs(0, 50);
    status = VL53L1X_SetROI(0, 8, 16);
    //  Start continuous ranging measurements
    status = VL53L1X_StartRanging(0);

    while (1) 
    {
        while (_DataReady == 0)
        {
            status = VL53L1X_CheckForDataReady(0, &_DataReady);
        }
        status = VL53L1X_GetRangeStatus(0, &_RangeStatus);
        status = VL53L1X_GetDistance(0, &_Distance);
        status = VL53L1X_ClearInterrupt(0);
        // //  Clear the display
        // SSD1306_Clear();
        // SSD1306_Render();
        //  Sleep for 1 second
        // _delay_ms(1000);
        // // //  Writhe my name on the display
        // SSD1306_StringXY(0, 2, "Andres");
        // SSD1306_StringXY(7, 2, "Tangarife");
        // SSD1306_Render();
        // //  Write the I2C scan results on the display
        // for (int i = 0; i < 128; i++)
        // {
        //     if (I2CScanResults[i] != 0)
        //     {
        //         char str[3];
        //         sprintf(str, "%02X", I2CScanResults[i]);
        //         SSD1306_StringXY(0, i, str);
        //         SSD1306_Render();
        //     }
        // }
        //  Display the distance as 10.00 cm
        sprintf(distance, "%u mm", _Distance);
        SSD1306_StringXY(0, 3, distance);
        SSD1306_Render();
        //  Reset the _Distance variable
        // _Distance = 0;
        // _DataReady = 0;
        //  sleep for 1 second
        _delay_ms(1000);

    }
}

