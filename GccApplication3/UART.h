#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <avr/interrupt.h>

// Se guardan las teclas recibidas mientras se actualiza la LCD.
volatile unsigned char rx[16];
volatile unsigned char entrada = 0, salida = 0;

void UART_init(void)
{
	UBRR0H = 0;
	UBRR0L = 103;              // 9600 baudios a 16 MHz
	UCSR0A = 0;
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
}

ISR(USART_RX_vect)
{
	unsigned char dato = UDR0;
	unsigned char siguiente = (entrada + 1) & 15;

	if (siguiente != salida)
	{
		rx[entrada] = dato;
		entrada = siguiente;
	}
}

unsigned char UART_read(void)
{
	unsigned char dato;
	if (entrada == salida) return 0;

	dato = rx[salida];
	salida = (salida + 1) & 15;
	return dato;
}

void UART_write(unsigned char dato)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = dato;
}

void UART_string(const char *texto)
{
	while (*texto) UART_write(*texto++);
}

#endif