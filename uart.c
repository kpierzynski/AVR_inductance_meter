/*
 * uart.c
 *
 *  Created on: 3 lut 2022
 *      Author: kpier
 */

#include "uart.h"

volatile uint8_t tx_buff[ TX_BUFF_SIZE ];
volatile uint8_t tx_tail;
volatile uint8_t tx_head;

void uart_init() {
	UCSRC |= ( 1 << UCSZ1 ) | ( 1 << UCSZ0 ); //8 - bit
	//UCSR0C &= ~( 1 << USBS0 ); //1 stop bit
	//UCSR0C &= ~( ( 1 << UPM00 ) | ( 1 << UPM01 ) ); //No parity

	//UCSR0A |= ( 1 << U2X0 ); //Boost speed

	//Set ubrr (desired baud)
	UBRRL = (uint8_t )( UBRR );
	UBRRH = (uint8_t )( UBRR >> 8 );

	//Turn on transmitter hardware
	UCSRB |= ( 1 << TXEN );
}

void uart_putc( uint8_t byte ) {
	uint8_t head = ( tx_head + 1 ) & ( TX_BUFF_SIZE - 1 );

	while( head == tx_tail )
		;

	tx_buff[ head ] = byte;
	tx_head = head;

	UCSRB |= ( 1 << UDRIE );
}

void uart_puts( char * c ) {
	while( *c )
		uart_putc( *c++ );
}

void uart_puts_P( const char *s )
{
	register char c;
	while( ( c = pgm_read_byte( s++ ) ) )
		uart_putc( c );
}

void uart_putb( uint8_t byte ) {
	uint8_t i = 0;
	for( i = 0; i < 8; i++ ) {
		if( byte & ( 1 << ( 8 - i - 1 ) ) )
			uart_putc( '1' );
		else
			uart_putc( '0' );
	}
}

void uart_puth( uint8_t byte ) {
	char c = '0';

	c += ( ( byte >> 4 ) & 0x0F );
	if( c > '9' )
		c = 'A' + ( c - '9' - 1 );
	uart_putc( c );

	c = '0';
	c += ( byte & 0x0F );
	if( c > '9' )
		c = 'A' + ( c - '9' - 1 );
	uart_putc( c );
}

void uart_putd( uint8_t byte ) {
	uart_putc( ( byte / 100 ) + '0' );
	uart_putc( ( byte / 10 ) % 10 + '0' );
	uart_putc( ( byte / 1 ) % 10 + '0' );
}

void uart_put_u16( uint16_t value ) {

	uart_putc( ( value / 10000 ) % 10+ '0' );
	uart_putc( ( value / 1000 ) % 10 + '0' );
	uart_putc( ( value / 100 ) % 10 + '0' );
	uart_putc( ( value / 10 ) % 10 + '0' );
	uart_putc( ( value / 1 ) % 10 + '0' );
}

void uart_putbuf( uint8_t * buf, uint8_t len, char * label ) {
	uint8_t i = 0;
	uart_puts( label );
	uart_putc( ':' );
	uart_putc( ' ' );

	for( i = 0; i < len; i++ ) {
		uart_puth( buf[ i ] );
	}
	uart_putc( '\r' );
	uart_putc( '\n' );
}

ISR( USART_UDRE_vect ) {
	uint8_t tail;

	if( tx_head != tx_tail ) {
		tail = ( tx_tail + 1 ) & ( TX_BUFF_SIZE - 1 );
		tx_tail = tail;
		UDR = tx_buff[ tail ];
	} else {
		UCSRB &= ~( 1 << UDRIE );
	}

}
