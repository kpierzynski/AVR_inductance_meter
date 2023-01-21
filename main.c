#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"

#define MAX_SAMPLES 5

volatile uint8_t samples_ready = 0;
volatile uint16_t samples[MAX_SAMPLES];

volatile uint8_t error = 0;

#define CHARGER PD5
#define CHARGER_DDR DDRD
#define CHARGER_PORT PORTD

#define CHARGER_HIGH CHARGER_PORT |= (1<<CHARGER);
#define CHARGER_LOW CHARGER_PORT &= ~(1<<CHARGER);

uint16_t avg(volatile uint16_t * array) {
	uint32_t sum = 0;
	for( uint8_t i = 1; i < MAX_SAMPLES; i++ ) {
		sum += samples[i];
	}

	return sum/(MAX_SAMPLES-1);
}

void run() {
	CHARGER_LOW;
	_delay_ms(1);
	TCNT1 = 0;
	CHARGER_HIGH;
	GIMSK |= (1<<INT0);
	TIMSK |= (1<<TOIE1);
}

int main() {
	CHARGER_DDR |= (1<<CHARGER);
	CHARGER_HIGH;
	_delay_ms(5000);

	uart_init();

	TCCR1B |= (1<<CS10);
	//TIMSK |= (1<<TOIE1);

	MCUCR |= (1<<ISC01);
	//GIMSK |= (1<<INT0);

	sei();
	uart_puts("started..\r\n");
	run();
	while(1) {
		if( samples_ready ) {
			samples_ready = 0;
			uint16_t s = avg(samples);
			uint32_t f = (uint32_t)s * 125 / 1000;
			uart_puth( (s >> 8) & 0xFF );
			uart_puth( (s >> 0) & 0xFF );
			uart_puts("\r\n");
			uart_puth( (f >> 8) & 0xFF );
			uart_puth( (f >> 0) & 0xFF );
			uart_puts("\r\n");
		}
		if( error ) {

			uart_puts("ERROR\r\n");

			error = 0;
		}
	}

	return 0;
}

ISR(INT0_vect) {
	static uint8_t i = 0;

	samples[i++] = TCNT1;

	if( i == MAX_SAMPLES ) {
		GIMSK &= ~(1<<INT0);
		TIMSK &= ~(1<<TOIE1);

		i = 0;
		samples_ready = 1;
	}
}

ISR(TIMER1_OVF_vect) {
	error = 1;
	GIMSK &= ~(1<<INT0);
	TIMSK &= ~(1<<TOIE1);
}
