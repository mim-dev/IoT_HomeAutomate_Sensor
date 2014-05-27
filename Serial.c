/*
 * Serial.c
 *
 * Created: 5/14/2014 3:31:17 PM
 *  Author: Luther
 */ 

#include <avr/io.h>
#include <stdint.h>

#include "Serial.h"

static const uint32_t MESSAGE_CATEGORY_VALUE = 0x001000A3;
static const uint8_t MESSAGE_DIRECTION_VALUE = 0x01;

static const uint8_t LAT = 7;
static const uint8_t LON = 6;
static const uint8_t TEMP = 5;
static const uint8_t BARO = 4;
static const uint8_t HUMIDITY = 3;
static const uint8_t WIND = 2;

static const uint32_t MANUFACTURER_IDENTITY_VALUE = 0x0000013D;
static const uint32_t SERIAL_NUMBER = 0x120067D3;

void initializeSerial(){
	
	// 9600 baud / 8Mhz clock
	UBRR0H = 0;
	UBRR0L = 51;
	
	// transmit only
	UCSR0B = (1 << TXEN0);
	
	// frame format: 8-N-1
	UCSR0C = (3 << UCSZ00);
	
}

void sendSerialData(int8_t latitudeDegrees, float latitudeMinutesSeconds, int8_t longitudeDegrees, float longitudeMinutesSeconds, float pressure)
{
	const uint8_t transmitSize = sizeof(uint8_t)	// message size
	+ sizeof(uint8_t)						// private segment start offset
	+ sizeof(uint8_t)						// message direction
	+ sizeof(uint32_t)						// message category
	+ sizeof(uint8_t)						//	--- BEGIN PRIVATE PAYLOAD ---	payload element valid flag
	+ sizeof(int8_t)						// degree element of latitude value
	+ sizeof(float)							// minutes / second element of latitude value
	+ sizeof(int8_t)						// degree element of longitude value
	+ sizeof(float)							// minutes / second element of latitude value
	+ sizeof(float)							// computed pressure value
	+ sizeof(uint32_t)						// sensor manufacturer value
	+ sizeof(uint32_t);						// sensor serial number value
	
	uint8_t transmitBuffer[sizeof(uint8_t)	// message size
	+ sizeof(uint8_t)						// private segment start offset
	+ sizeof(uint8_t)						// message direction
	+ sizeof(uint32_t)						// message category
	+ sizeof(uint8_t)						//	--- BEGIN PRIVATE PAYLOAD ---	payload element valid flag
	+ sizeof(int8_t)						// degree element of latitude value
	+ sizeof(float)							// minutes / second element of latitude value
	+ sizeof(int8_t)						// degree element of longitude value
	+ sizeof(float)							// minutes / second element of latitude value
	+ sizeof(float)							// computed pressure value
	+ sizeof(uint32_t)						// sensor manufacturer value
	+ sizeof(uint32_t)];					// sensor serial number value
	
	uint8_t transmitBufferIndex = 0;
	uint8_t elementByteIndex;
	
	transmitBuffer[transmitBufferIndex++] = transmitSize;
	transmitBuffer[transmitBufferIndex++] = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + 1;
	transmitBuffer[transmitBufferIndex++] = MESSAGE_DIRECTION_VALUE;
	
	uint8_t *ptrPayloadElementStart = (uint8_t *)&MESSAGE_CATEGORY_VALUE;
	
	for(elementByteIndex = 0; elementByteIndex < sizeof MESSAGE_CATEGORY_VALUE; elementByteIndex++)
	{
		transmitBuffer[transmitBufferIndex++] = *(ptrPayloadElementStart + elementByteIndex);
	}
	
	transmitBuffer[transmitBufferIndex++] = (1 << LAT) | (1 << LON) | (1 << BARO);
	transmitBuffer[transmitBufferIndex++] = latitudeDegrees;

	ptrPayloadElementStart = (uint8_t *)&latitudeMinutesSeconds;
	for(elementByteIndex = 0; elementByteIndex < sizeof latitudeMinutesSeconds; elementByteIndex++)
	{
		transmitBuffer[transmitBufferIndex++] = *(ptrPayloadElementStart + elementByteIndex);
	}
	
	transmitBuffer[transmitBufferIndex++] = longitudeDegrees;
	
	ptrPayloadElementStart = (uint8_t *)&longitudeMinutesSeconds;
	for(elementByteIndex = 0; elementByteIndex < sizeof longitudeMinutesSeconds; elementByteIndex++)
	{
		transmitBuffer[transmitBufferIndex++] = *(ptrPayloadElementStart + elementByteIndex);
	}
	
	ptrPayloadElementStart = (uint8_t *)&pressure;
	for(elementByteIndex = 0; elementByteIndex < sizeof pressure; elementByteIndex++)
	{
		transmitBuffer[transmitBufferIndex++] = *(ptrPayloadElementStart + elementByteIndex);
	}
	
	ptrPayloadElementStart = (uint8_t *)&MANUFACTURER_IDENTITY_VALUE;
	for(elementByteIndex = 0; elementByteIndex < sizeof MANUFACTURER_IDENTITY_VALUE; elementByteIndex++)
	{
		transmitBuffer[transmitBufferIndex++] = *(ptrPayloadElementStart + elementByteIndex);
	}
	
	ptrPayloadElementStart = (uint8_t *)&SERIAL_NUMBER;
	for(elementByteIndex = 0; elementByteIndex < sizeof SERIAL_NUMBER; elementByteIndex++)
	{
		transmitBuffer[transmitBufferIndex++] = *(ptrPayloadElementStart + elementByteIndex);
	}
	
	for(uint8_t x = 0; x < transmitBufferIndex; x ++){
		while(!(UCSR0A & (1 << UDRE0)));
		UDR0 = *(transmitBuffer + x);
	}
}