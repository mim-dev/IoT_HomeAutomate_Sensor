/*
 * HomeAutomateBaro.c
 *
 * Created: 5/12/2014 6:16:08 PM
 *  Author: Luther
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "MPL115A1.h"
#include "Serial.h"

static volatile uint8_t TIMER_1_COUNT = 0;

void clearAndStartTimer();

int main(void)
{
	// set up the sensor & serial
	initializeMPL115A1Sensor();
	initializeSerial();
	
	// enable external INT0 and set trigger on falling edge
	cli();
	
	EICRA |= 1 << ISC01;
	EIMSK |= 1 << INT0;
	
	// set up the timer
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
	TIMSK1 = (1 << OCIE1A);
	OCR1A = 39062;	// 5 second compare match
	
	sei();

	
	// enable visual indicators
	DDRD |= (1 << PIND0);
	
    while(1)
    {
        //TODO:: Please write your application code 
    }
}

void clearAndStartTimer(){
	
	uint8_t sReg = SREG;
	
	cli();
	
	TCNT1 = 0x0000;
	SREG = sReg;
	
	TCCR1B |= (1 << CS12) | (1 << CS10);
}

ISR(TIMER1_COMPA_vect){
	
	if(TIMER_1_COUNT == 179){	// 15 minute interval
		
		// stop the timer and reset the count
		TCCR1B &= 0xFF ^ ((1 << CS12) | (1 << CS11) | (1 << CS10));
		TIMER_1_COUNT = 0;
		
		// provide visual indication of what is about to happen
		for(int x = 0; x < 2; x++){
			PORTD ^= (1 << PIND0);
			_delay_ms(100);
			PORTD ^= (1 << PIND0);
			_delay_ms(100);
		}
		float pressure = sampleMPL115A1Sensor();
	
		// hard code a locust grove, ga latitude
		sendSerialData(33, 23.6587, -84, 5.8400, pressure);
		
		// clear the INT1 and Timer1 Output Compare 1 flags in the event either has been triggered while handling interrupt
		EIFR |= (1 << INTF0);
		TIFR1 |= (1 << OCF1A);
		
		clearAndStartTimer();
		
	} else{
		TIMER_1_COUNT += 1;
	}
}

ISR(INT0_vect){
	
	// stop the timer and reset the count
	TCCR1B &= 0xFF ^ ((1 << CS12) | (1 << CS11) | (1 << CS10));
	TIMER_1_COUNT = 0;
	
	// provide visual indication of what is about to happen
	for(int x = 0; x < 2; x++){
		PORTD ^= (1 << PIND0);
		_delay_ms(100);
		PORTD ^= (1 << PIND0);
		_delay_ms(100);
	}
		
	float pressure = sampleMPL115A1Sensor();
	
	// hard code a locust grove, ga latitude
	sendSerialData(33, 23.6587, -84, 5.8400, pressure);
	
	// clear the INT1 and Timer1 Output Compare 1 flags in the event either has been triggered while handling interrupt
	EIFR |= (1 << INTF0);
	TIFR1 |= (1 << OCF1A);	
	
	clearAndStartTimer();
}

