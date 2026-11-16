
/* 
* This file is part of VL53L1 Platform 
* 
* Copyright (c) 2016, STMicroelectronics - All Rights Reserved 
* 
* License terms: BSD 3-clause "New" or "Revised" License. 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met: 
* 
* 1. Redistributions of source code must retain the above copyright notice, this 
* list of conditions and the following disclaimer. 
* 
* 2. Redistributions in binary form must reproduce the above copyright notice, 
* this list of conditions and the following disclaimer in the documentation 
* and/or other materials provided with the distribution. 
* 
* 3. Neither the name of the copyright holder nor the names of its contributors 
* may be used to endorse or promote products derived from this software 
* without specific prior written permission. 
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
* 
*/


/*
 * ATmega328P port of the VL53L1X platform layer.
 * Every register access goes through the polling I2C driver in lib/src/I2C328P.c.
 * The VL53L1X uses 16-bit register indexes and big-endian (MSB first) data.
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include "vl53l1_platform.h"
#include <util/delay.h>
#include "I2C.h"

// 7-bit I2C address of the VL53L1X (0x52 >> 1). The ST API passes a 'dev'
// handle to every call; this port supports a single sensor at the default address.
#define VL53L1X_I2C_ADDR 0x29

// Open a write transaction and send the 16-bit register index,
// optionally ending the transaction with STOP.
static int8_t VL53L1_SetIndex(uint16_t index, int bStop)
{
	if (I2C_Start(VL53L1X_I2C_ADDR, I2C_WRITE))
		return -1;

	if (I2C_Write8((unsigned char)(index >> 8), I2C_NOSTOP))
		return -2;

	if (I2C_Write8((unsigned char)index, bStop))
		return -2;

	return 0;
}

// Write 'count' bytes starting at register 'index'; the last byte ends with STOP.
static int8_t VL53L1_Write(uint16_t index, const uint8_t *pdata, uint32_t count)
{
	if (count == 0)
		return -3;

	if (VL53L1_SetIndex(index, I2C_NOSTOP))
		return -1;

	for (uint32_t i = 0; i < count; i++)
	{
		if (I2C_Write8(pdata[i], (i == count - 1) ? I2C_STOP : I2C_NOSTOP))
			return -3;
	}

	return 0;
}

// Read 'count' bytes starting at register 'index'.
// Every byte is ACKed except the last one, which is NACKed and followed by STOP.
static int8_t VL53L1_Read(uint16_t index, uint8_t *pdata, uint32_t count)
{
	if (count == 0)
		return -3;

	// set the register index, then STOP
	if (VL53L1_SetIndex(index, I2C_STOP))
		return -1;

	// new START with intent to read
	if (I2C_Start(VL53L1X_I2C_ADDR, I2C_READ))
		return -3;

	for (uint32_t i = 0; i < count; i++)
	{
		int8_t last = (i == count - 1);
		if (I2C_Read8(&pdata[i], last ? I2C_NACK : I2C_ACK, last ? I2C_STOP : I2C_NOSTOP))
			return -4;
	}

	return 0;
}

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count)
{
	(void)dev;
	return VL53L1_Write(index, pdata, count);
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count)
{
	(void)dev;
	return VL53L1_Read(index, pdata, count);
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data)
{
	(void)dev;
	return VL53L1_Write(index, &data, 1);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data)
{
	(void)dev;
	uint8_t buff[2] = { (uint8_t)(data >> 8), (uint8_t)data };
	return VL53L1_Write(index, buff, 2);
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data)
{
	(void)dev;
	uint8_t buff[4] = { (uint8_t)(data >> 24), (uint8_t)(data >> 16),
	                    (uint8_t)(data >> 8),  (uint8_t)data };
	return VL53L1_Write(index, buff, 4);
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data)
{
	(void)dev;
	return VL53L1_Read(index, data, 1);
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data)
{
	(void)dev;
	uint8_t buff[2];
	int8_t status = VL53L1_Read(index, buff, 2);
	if (status)
		return status;

	// first byte received is the MSB
	*data = ((uint16_t)buff[0] << 8) | buff[1];
	return 0;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data)
{
	(void)dev;
	uint8_t buff[4];
	int8_t status = VL53L1_Read(index, buff, 4);
	if (status)
		return status;

	// cast before shifting: int is only 16 bits on AVR
	*data = ((uint32_t)buff[0] << 24) | ((uint32_t)buff[1] << 16) |
	        ((uint32_t)buff[2] << 8)  |  (uint32_t)buff[3];
	return 0;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms)
{
	(void)dev;
	// _delay_ms needs a compile-time constant, so wait 1 ms at a time
	while (wait_ms-- > 0)
		_delay_ms(1);
	return 0;
}
