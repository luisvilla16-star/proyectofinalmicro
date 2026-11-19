/*
 * LCD.c
 *
 * LCD 16x2
 * ATmega328P
 * Modo 4 bits
 *
 * RS -> PC0
 * E  -> PC1
 * D4 -> PC2
 * D5 -> PC3
 * D6 -> PC4
 * D7 -> PC5
 */

#include "LCD.h"

// =====================================================
// ENVIAR 4 BITS
// =====================================================

static void lcd_nibble(uint8_t nibble)
{
    // Limpiar PC2-PC5
    DATA_BUS &=
        ~((1 << LCD_D4) |
          (1 << LCD_D5) |
          (1 << LCD_D6) |
          (1 << LCD_D7));

    // Colocar bit 0 en D4
    if (nibble & 0x01)
        DATA_BUS |= (1 << LCD_D4);

    // Colocar bit 1 en D5
    if (nibble & 0x02)
        DATA_BUS |= (1 << LCD_D5);

    // Colocar bit 2 en D6
    if (nibble & 0x04)
        DATA_BUS |= (1 << LCD_D6);

    // Colocar bit 3 en D7
    if (nibble & 0x08)
        DATA_BUS |= (1 << LCD_D7);

    // Pulso ENABLE
    CTL_BUS |= (1 << LCD_EN);

    _delay_us(2);

    CTL_BUS &= ~(1 << LCD_EN);

    _delay_us(50);
}

// =====================================================
// ENVIAR BYTE
// =====================================================

static void lcd_byte(uint8_t dato, uint8_t caracter)
{
    if (caracter)
    {
        CTL_BUS |= (1 << LCD_RS);
    }
    else
    {
        CTL_BUS &= ~(1 << LCD_RS);
    }

    // Parte alta
    lcd_nibble(dato >> 4);

    // Parte baja
    lcd_nibble(dato & 0x0F);
}

// =====================================================
// INICIALIZAR LCD
// =====================================================

void lcd_init(void)
{
    // PC0-PC5 como salidas
    DATA_DDR |=
        (1 << LCD_RS) |
        (1 << LCD_EN) |
        (1 << LCD_D4) |
        (1 << LCD_D5) |
        (1 << LCD_D6) |
        (1 << LCD_D7);

    // Inicialmente todo LOW
    DATA_BUS &=
        ~((1 << LCD_RS) |
          (1 << LCD_EN) |
          (1 << LCD_D4) |
          (1 << LCD_D5) |
          (1 << LCD_D6) |
          (1 << LCD_D7));

    _delay_ms(50);

    // RS = 0
    CTL_BUS &= ~(1 << LCD_RS);

    // E = 0
    CTL_BUS &= ~(1 << LCD_EN);

    // =================================================
    // INICIALIZACION HD44780
    // =================================================

    lcd_nibble(0x03);

    _delay_ms(5);

    lcd_nibble(0x03);

    _delay_us(150);

    lcd_nibble(0x03);

    _delay_us(150);

    // Modo 4 bits
    lcd_nibble(0x02);

    _delay_us(150);

    // 4 bits - 2 filas
    lcd_send_command(
        LCD_CMD_4BIT_2ROW_5X7
    );

    // Display apagado
    lcd_send_command(
        LCD_CMD_DISPLAY_OFF
    );

    // Limpiar pantalla
    lcd_clear();

    // Cursor avanza
    lcd_send_command(
        LCD_CMD_ENTRY_INCREMENT
    );

    // Display encendido
    lcd_send_command(
        LCD_CMD_DISPLAY_NO_CURSOR
    );
}

// =====================================================
// ENVIAR COMANDO
// =====================================================

void lcd_send_command(uint8_t command)
{
    lcd_byte(command, 0);

    if (command == LCD_CMD_CLEAR_DISPLAY ||
        command == LCD_CMD_CURSOR_HOME)
    {
        _delay_ms(2);
    }
    else
    {
        _delay_us(50);
    }
}

// =====================================================
// ESCRIBIR CARACTER
// =====================================================

void lcd_write_character(uint8_t character)
{
    lcd_byte(character, 1);

    _delay_us(50);
}

// =====================================================
// ESCRIBIR PALABRA
// =====================================================

void lcd_write_word(const char *word)
{
    while (*word != '\0')
    {
        lcd_write_character(
            (uint8_t)*word
        );

        word++;
    }
}

// =====================================================
// LIMPIAR LCD
// =====================================================

void lcd_clear(void)
{
    lcd_send_command(
        LCD_CMD_CLEAR_DISPLAY
    );

    _delay_ms(2);
}

// =====================================================
// POSICION DEL CURSOR
// =====================================================

void lcd_goto_xy(uint8_t line, uint8_t pos)
{
    if (line == 0)
    {
        lcd_send_command(
            0x80 + pos
        );
    }
    else
    {
        lcd_send_command(
            0xC0 + pos
        );
    }
}