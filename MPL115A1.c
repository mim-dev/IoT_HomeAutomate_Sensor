/*
 * MPL115A1.c
 *
 * Created: 5/12/2014 6:17:07 PM
 *  Author: Luther
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <math.h>

#include "MPL115A1.h"

static const char SS_HIGH_MASK = (1 << PINB2);
static const char SS_LOW_MASK = (0xFF ^ (1 << PINB2));

static const int SIGN_BIT_MASK = 0x8000;

static const int A0_COEFFICIENT_FRACTION_BITS = 3;

static const int B1_COEFFICIENT_FRACTION_BITS = 13;

static const int B2_COEFFICIENT_FRACTION_BITS = 14;

static const int C12_COEFFICIENT_FRACTION_BITS = 13;
static const int C12_COEFFICIENT_PADDING_FRACTION_BITS = 9;

static const int COEFFICIENT_BYTE_COUNT = 8;

typedef struct MPL115A1Coefficients {
	
	float a0Coefficient;
	float b1Coefficient;
	float b2Coefficient;
	float c12Coefficient;
} MPL115A1Coefficients;

static MPL115A1Coefficients mpl115A1Coefficients;

// forward declarations
void readCoefficentValues(char *coefficientValueBuffer);
char readSensorValue(char valueAddress);
void clearSensorLastDataByte();
void computeCoefficients(char *coefficientValueBuffer);
float computeCompensatedPressure(int rawTemperature, int rawPressure);

void initializeMPL115A1Sensor(){
	
	// initialize the SPI
	DDRB = (1 << PINB2) | (1 << PINB3) | (1 << PINB5);		// SS, MOSI, SCK as outputs
	SPCR = ((1 << SPE) | (1<< MSTR));						// SPI = enabled, Master / Slave = Master, Mode = 0, Polling, Clk = Fosc / 4

	char rawCoefficientValues[COEFFICIENT_BYTE_COUNT];
	readCoefficentValues(rawCoefficientValues);
	computeCoefficients(rawCoefficientValues);	
}

float sampleMPL115A1Sensor(){
	
	// start pressure / temperature conversion
	PORTB &= SS_LOW_MASK;		// (chip) select the sensor
	SPDR = 0x24;
	while(!(SPSR & (1 << SPIF)));
	SPDR = 0x00;
	while(!(SPSR & (1 << SPIF)));
	PORTB |= SS_HIGH_MASK;		// (chip) de-select the sensor

	_delay_ms(3);
	
	PORTB &= SS_LOW_MASK;		// (chip) select the sensor
	char rawPressureMSB = readSensorValue(0x80);
	char rawPressureLSB = readSensorValue(0x82);
	char rawTemperatureMSB = readSensorValue(0x84);
	char rawTemperatureLSB = readSensorValue(0x86);
	clearSensorLastDataByte();
	PORTB |= SS_HIGH_MASK;		// (chip) de-select the sensor
	
	int rawPressure = (rawPressureMSB << 8) | rawPressureLSB;
	int rawTemperature = (rawTemperatureMSB << 8) | rawTemperatureLSB;
	
	return computeCompensatedPressure(rawTemperature, rawPressure);
}

// internal functions

void readCoefficentValues(char *coefficientValueBuffer){
	
	char baseCoefficientAdressReadCommand = 0x88;
	
	PORTB &= SS_LOW_MASK;		// (chip) select the sensor
	
	for(int x = 0; x < 8; x++){
		*(coefficientValueBuffer + (sizeof(char) * x)) = readSensorValue(baseCoefficientAdressReadCommand);
		baseCoefficientAdressReadCommand += 2;
		 
	}
	
	clearSensorLastDataByte();
	
	PORTB |= SS_HIGH_MASK;		// (chip) de-select the sensor
}

char readSensorValue(char valueAddress){
	
	char coefficentValue;
	
	SPDR = valueAddress;
	while(!(SPSR & (1 << SPIF)));
	SPDR = 0x00;
	while(!(SPSR & (1 << SPIF)));
	coefficentValue = SPDR;
	
	return coefficentValue;
}

void clearSensorLastDataByte(){
	SPDR = 0x00;
	while(!(SPSR & (1 << SPIF)));
}

void computeCoefficients(char *coefficientValueBuffer){
	
	int a0CoefficientMSBIndex = 0;
	int a0CoefficientLSBIndex = 1;
	int b1CoefficientMSBIndex = 2;
	int b1CoefficientLSBIndex = 3;
	int b2CoefficientMSBIndex = 4;
	int b2CoefficientLSBIndex = 5;
	int c12CoefficientMSBIndex = 6;
	int c12CoefficientLSBIndex = 7;
		
	// a0 coefficient processing
	int a0CoefficientRawValue = (coefficientValueBuffer[a0CoefficientMSBIndex] << 8) | coefficientValueBuffer[a0CoefficientLSBIndex];
	int a0CoefficientRawSignValue = a0CoefficientRawValue & 0x8000;
	
	// if sign is negative convert back to one's compliment
	if(a0CoefficientRawSignValue & SIGN_BIT_MASK){
		a0CoefficientRawValue = ~ (a0CoefficientRawValue - 1);
	}
	
	int a0CoefficientRawDecimalValue = (a0CoefficientRawValue & 0x0007);
	float a0CoefficientDecimalElement = a0CoefficientRawDecimalValue * pow(2, (A0_COEFFICIENT_FRACTION_BITS * -1));
	
	int a0CoefficentRawIntegerValue = (a0CoefficientRawValue & 0x7FF8) >> A0_COEFFICIENT_FRACTION_BITS;
	mpl115A1Coefficients.a0Coefficient = (a0CoefficentRawIntegerValue + a0CoefficientDecimalElement) * ((a0CoefficientRawSignValue & SIGN_BIT_MASK) ? -1.0 : 1.0);
	
	// b1 coefficient processing
	int b1CoefficientRawValue = (coefficientValueBuffer[b1CoefficientMSBIndex] << 8) | coefficientValueBuffer[b1CoefficientLSBIndex];
	int b1CoefficientRawSignValue = b1CoefficientRawValue & SIGN_BIT_MASK;
	
	// if sign negative convert back to one's compliment
	if(b1CoefficientRawSignValue & SIGN_BIT_MASK){
		b1CoefficientRawValue = ~(b1CoefficientRawValue - 1);
	}
	
	int b1CoefficientRawDecimalValue = (b1CoefficientRawValue & 0x1FFF);
	float b1CoefficientDecimalElement = b1CoefficientRawDecimalValue * pow(2, (B1_COEFFICIENT_FRACTION_BITS * -1));
	
	int b1CoefficentRawIntegerValue = (b1CoefficientRawValue & 0x4000) >> B1_COEFFICIENT_FRACTION_BITS;
	mpl115A1Coefficients.b1Coefficient = (b1CoefficentRawIntegerValue + b1CoefficientDecimalElement) * ((b1CoefficientRawSignValue & SIGN_BIT_MASK) ? -1.0 : 1.0);
	
	// b2 coefficient processing
	int b2CoefficientRawValue = (coefficientValueBuffer[b2CoefficientMSBIndex] << 8) | coefficientValueBuffer[b2CoefficientLSBIndex];
	int b2CoefficientRawSignValue = b2CoefficientRawValue & SIGN_BIT_MASK;
	
	// if sign negative convert back to one's compliment
	if(b2CoefficientRawSignValue & SIGN_BIT_MASK){
		b2CoefficientRawValue = ~(b2CoefficientRawValue - 1);
	}
	
	int b2CoefficientRawDecimalValue = (b2CoefficientRawValue & 0x3FFF);
	float b2CoefficientDecimalElement = b2CoefficientRawDecimalValue * pow(2, (B2_COEFFICIENT_FRACTION_BITS * -1));
	
	int b2CoefficentRawIntegerValue = (b2CoefficientRawValue & 0x4000) >> B2_COEFFICIENT_FRACTION_BITS;
	mpl115A1Coefficients.b2Coefficient = (b2CoefficentRawIntegerValue + b2CoefficientDecimalElement) * ((b2CoefficientRawSignValue & SIGN_BIT_MASK) ? -1.0 : 1.0);
	
	// c12 coefficient processing
	int c12CoefficientRawValue = (coefficientValueBuffer[c12CoefficientMSBIndex] << 8) | coefficientValueBuffer[c12CoefficientLSBIndex];
	int c12CoefficientRawSignValue = c12CoefficientRawValue & SIGN_BIT_MASK;
	
	// if sign negative convert back to one's compliment
	if(c12CoefficientRawSignValue & SIGN_BIT_MASK){
		c12CoefficientRawValue = ~(c12CoefficientRawValue - 1);
	}
	
	int c12CoefficientRawDecimalValue = (c12CoefficientRawValue & 0x7FFF) >> 2;  // 2 LSB for c12 coefficient are don't care
	float c12CoefficientDecimalElement = c12CoefficientRawDecimalValue * pow(2, ((C12_COEFFICIENT_FRACTION_BITS + C12_COEFFICIENT_PADDING_FRACTION_BITS) * -1));
	
	mpl115A1Coefficients.c12Coefficient = c12CoefficientDecimalElement * ((c12CoefficientRawSignValue & SIGN_BIT_MASK) ? -1.0 : 1.0);
}

float computeCompensatedPressure(int rawTemperature, int rawPressure){
	
	int temperature = (rawTemperature >> 6) & 0x03FF;
	int pressure = (rawPressure >> 6) & 0x03FF;
	
	float c12x2 = mpl115A1Coefficients.c12Coefficient * (float)temperature;
	float a1 = mpl115A1Coefficients.b1Coefficient + c12x2;
	float a1x1 = a1 * (float)pressure;
	float y1 = mpl115A1Coefficients.a0Coefficient + a1x1;
	float a2x2 = mpl115A1Coefficients.b2Coefficient * (float)temperature;
	float compensatedPressure = y1 + a2x2;
	
	return compensatedPressure * ((115.0 - 50.0) / 1023.0) + 50.0;
}