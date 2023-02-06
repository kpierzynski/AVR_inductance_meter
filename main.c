#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"

#define MAX_SAMPLES 15

#define ABS(x) (((x) < 0) ? (-(x)) : (x))

volatile uint8_t samples_ready = 0;
volatile uint16_t samples[MAX_SAMPLES];

volatile uint8_t error = 0;

#define CHARGER PD5
#define CHARGER_DDR DDRD
#define CHARGER_PORT PORTD

#define CHARGER_HIGH CHARGER_PORT |= (1 << CHARGER);
#define CHARGER_LOW CHARGER_PORT &= ~(1 << CHARGER);

uint32_t average(volatile uint16_t *arr)
{
	uint8_t i, count = 0;
	uint32_t sum = 0;
	uint32_t avg, stddev = 0;

	// calculate average
	for (i = 0; i < MAX_SAMPLES; i++)
	{
		sum += arr[i];
	}
	avg = sum / MAX_SAMPLES;

	// calculate standard deviation
	for (i = 0; i < MAX_SAMPLES; i++)
	{
		stddev += (arr[i] - avg) * (arr[i] - avg);
	}
	stddev = stddev / MAX_SAMPLES;

	// filter values
	for (i = 0; i < MAX_SAMPLES; i++)
	{
		if (ABS(arr[i] - avg) < 3 * stddev)
		{
			sum += arr[i];
			count++;
		}
	}

	avg = sum / count;

	return avg;
}

uint16_t avg(volatile uint16_t *array)
{
	uint32_t sum = 0;
	uint8_t div = 0;
	for (uint8_t i = 0; i < MAX_SAMPLES; i++)
	{
		if (samples[i] < 2000 || samples[i] > 3000)
			continue;

		sum += samples[i];
		div++;
	}

	return sum / (div);
}

void run()
{
	CHARGER_LOW;
	_delay_us(500);
	TCNT1 = 0;
	CHARGER_HIGH;
	GIMSK |= (1 << INT0);
	TIMSK |= (1 << TOIE1);
}

int main()
{
	CHARGER_DDR |= (1 << CHARGER);
	CHARGER_HIGH;
	_delay_ms(5000);

	uart_init();

	TCCR1B |= (1 << CS10);
	// TIMSK |= (1<<TOIE1);

	MCUCR = (MCUCR & 0xF0) | (1 << ISC01);
	// GIMSK |= (1<<INT0);

	sei();
	uart_puts("started..\r\n");

	run();
	while (1)
	{
		if (samples_ready)
		{
			samples_ready = 0;
			uart_putc('[');
			for (uint8_t i = 0; i < MAX_SAMPLES; i++)
			{
				uart_put_u16(samples[i]);
				uart_putc(',');
				uart_putc(' ');
			}
			uart_putc(']');
			uart_puts("\r\n");
			uint16_t s = average(samples);
			uint32_t f = (uint32_t)s * 125 / 2 / 1000;
			uart_put_u16(s);
			uart_puts("\r\n");
			uart_put_u16(f);
			uart_puts("uS\r\n");
		}
		if (error)
		{
			uart_puts("ERROR\r\n");
			error = 0;
		}
	}

	return 0;
}

ISR(INT0_vect)
{
	static uint8_t i = 0;

	samples[i] = TCNT1;
	i++;

	TCNT1 = 0;

	if (i == MAX_SAMPLES)
	{
		GIMSK &= ~(1 << INT0);
		TIMSK &= ~(1 << TOIE1);

		i = 0;
		samples_ready = 1;
	}
}

ISR(TIMER1_OVF_vect)
{
	error = 1;
	GIMSK &= ~(1 << INT0);
	TIMSK &= ~(1 << TOIE1);
}
