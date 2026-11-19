/*
 * LCD.h
 *
 * LCD 16x2 - ATmega328P
 * Modo 4 bits
 *
 * CONEXIONES INICIALES:
 *
 * RS -> PC0 / A0
 * E  -> PC1 / A1
 * D4 -> PC2 / A2
 * D5 -> PC3 / A3
 * D6 -> PC4 / A4
 * D7 -> PC5 / A5
 * RW -> GND
 */

#ifndef LCD_H_
#define LCD_H_

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

// =====================================================
// LCD EN PORTC
// =====================================================

#define DATA_BUS PORTC
#define CTL_BUS  PORTC

#define DATA_DDR DDRC
#define CTL_DDR  DDRC

#define LCD_RS PC0
#define LCD_EN PC1

#define LCD_D4 PC2
#define LCD_D5 PC3
#define LCD_D6 PC4
#define LCD_D7 PC5

// =====================================================
// COMANDOS
// =====================================================

#define LCD_CMD_CLEAR_DISPLAY           0x01
#define LCD_CMD_CURSOR_HOME             0x02

#define LCD_CMD_ENTRY_INCREMENT         0x06

#define LCD_CMD_DISPLAY_OFF             0x08
#define LCD_CMD_DISPLAY_NO_CURSOR       0x0C
#define LCD_CMD_DISPLAY_CURSOR_NO_BLINK 0x0E
#define LCD_CMD_DISPLAY_CURSOR_BLINK    0x0F

#define LCD_CMD_4BIT_2ROW_5X7           0x28
#define LCD_CMD_8BIT_2ROW_5X7           0x38

// =====================================================
// PROTOTIPOS
// =====================================================

void lcd_init(void);

void lcd_send_command(uint8_t command);

void lcd_write_character(uint8_t character);

void lcd_write_word(const char *word);

void lcd_clear(void);

void lcd_goto_xy(uint8_t line, uint8_t pos);

#endif