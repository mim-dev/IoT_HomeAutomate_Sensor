/*
 * Serial.h
 *
 * Created: 5/14/2014 3:31:02 PM
 *  Author: Luther
 */ 


#ifndef SERIAL_H_
#define SERIAL_H_


void initializeSerial();
void sendSerialData(int8_t latitudeDegrees, float latitudeMinutesSeconds, int8_t longitudeDegrees, float longitudeMinutesSeconds, float pressure);


#endif /* SERIAL_H_ */