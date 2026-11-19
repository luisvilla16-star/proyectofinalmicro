#ifndef EEPROM_H_
#define EEPROM_H_

#include <avr/io.h>
#include <avr/interrupt.h>

unsigned char EEPROM_read(unsigned int direccion)
{
	while (EECR & (1 << EEPE));
	EEAR = direccion;
	EECR |= (1 << EERE);
	return EEDR;
}

void EEPROM_write(unsigned int direccion, unsigned char dato)
{
	unsigned char estado;

	while (EECR & (1 << EEPE));
	EEAR = direccion;
	EEDR = dato;

	estado = SREG;
	cli();
	EECR |= (1 << EEMPE);
	EECR |= (1 << EEPE);
	SREG = estado;
}

void EEPROM_update(unsigned int direccion, unsigned char dato)
{
	if (EEPROM_read(direccion) != dato)
	EEPROM_write(direccion, dato);
}

#endif