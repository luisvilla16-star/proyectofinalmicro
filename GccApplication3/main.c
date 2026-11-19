
/*
 * PATTERNIX
 * ATmega328P - Microchip Studio - C puro
 *
 * Juego 1: Sucesiones numericas
 * Juego 2: Calculadora
 *
 * LCD: PORTC
 * Buzzer: PB0
 * UART: PD0 y PD1
 * Teclado: PD2-PD7 y PB1-PB2
 *
 * Timer1: interrupcion cada 1 ms
 * UART: interrupcion de recepcion
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdint.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "LCD.h"
#include "UART.h"
#include "EEPROM.h"

#define BUZZER PB0

#define NUM_RECORDS 5
#define NUM_PREGUNTAS 5

// =====================================================
// VARIABLES GLOBALES
// =====================================================

volatile uint32_t milis = 0;

uint8_t dificultad = 1;
uint8_t juego = 0;

uint16_t numero1;
uint16_t numero2;
uint16_t resultado;

char operacion;

uint16_t sucesion[5];
uint16_t respuestaCorrecta;

// =====================================================
// TIMER 1 - INTERRUPCION CADA 1 MILISEGUNDO
// =====================================================

ISR(TIMER1_COMPA_vect)
{
    milis++;
}

uint32_t tiempoActual(void)
{
    uint32_t valor;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        valor = milis;
    }

    return valor;
}

void iniciarTimer(void)
{
    TCCR1A = 0;
    TCCR1B = 0;

    TCNT1 = 0;
    OCR1A = 249;

    TCCR1B = (1 << WGM12) |
             (1 << CS11) |
             (1 << CS10);

    TIMSK1 |= (1 << OCIE1A);
}

void esperarTiempo(uint32_t duracion)
{
    uint32_t inicio = tiempoActual();

    while ((uint32_t)(tiempoActual() - inicio) < duracion)
    {
    }
}

// =====================================================
// TECLADO MATRICIAL
// =====================================================

// Filas: PD2, PD3, PD4, PD5
// Columnas: PD6, PD7, PB1, PB2

char mapa[4][4] =
{
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

void teclado_init(void)
{
	// Filas: D2, D3, D4, D5 como salidas
	DDRD |= (1 << PD2) |
	(1 << PD3) |
	(1 << PD4) |
	(1 << PD5);

	// Columnas: D6 y D7 como entradas
	DDRD &= ~((1 << PD6) | (1 << PD7));

	// Columnas: D9 y D10 como entradas
	DDRB &= ~((1 << PB1) | (1 << PB2));

	// Activar resistencias pull-up
	PORTD |= (1 << PD6) | (1 << PD7);

	PORTB |= (1 << PB1) | (1 << PB2);

	// Todas las filas inicialmente en HIGH
	PORTD |= (1 << PD2) |
	(1 << PD3) |
	(1 << PD4) |
	(1 << PD5);
}



char buscarTecla(void)
{
	uint8_t fila;
	char tecla = 0;

	uint8_t filas[4] = {PD2, PD3, PD4, PD5};

	for (fila = 0; fila < 4; fila++)
	{
		// Todas las filas en HIGH
		PORTD |= (1 << PD2) |
		(1 << PD3) |
		(1 << PD4) |
		(1 << PD5);

		// Activar fila actual
		PORTD &= ~(1 << filas[fila]);

		_delay_us(10);

		if (!(PIND & (1 << PD6)))
		tecla = mapa[fila][0];

		else if (!(PIND & (1 << PD7)))
		tecla = mapa[fila][1];

		else if (!(PINB & (1 << PB1)))
		tecla = mapa[fila][2];

		else if (!(PINB & (1 << PB2)))
		tecla = mapa[fila][3];

		// Restaurar fila
		PORTD |= (1 << filas[fila]);

		if (tecla != 0)
		return tecla;
	}

	return 0;
}




char leerTecla(void)
{
	char tecla;

	while (1)
	{
		tecla = buscarTecla();

		if (tecla != 0)
		{
			esperarTiempo(25);

			// Comprobar que sigue presionada
			if (buscarTecla() == tecla)
			{
				// Esperar liberacion
				while (buscarTecla() != 0)
				{
					esperarTiempo(5);
				}

				// Antirrebote al liberar
				esperarTiempo(20);

				return tecla;
			}
		}
	}
}


// =====================================================
// BUZZER
// =====================================================

void beep(void)
{
	PORTB |= (1 << BUZZER);

	esperarTiempo(80);

	PORTB &= ~(1 << BUZZER);
}

void beepError(void)
{
	PORTB |= (1 << BUZZER);

	esperarTiempo(200);

	PORTB &= ~(1 << BUZZER);
}

// =====================================================
// MOSTRAR Y ENVIAR NUMEROS
// =====================================================

void mostrarNumero(uint16_t numero)
{
    char texto[6];

    itoa(numero, texto, 10);

    lcd_write_word(texto);
}

void enviarNumero(uint16_t numero)
{
    char texto[6];

    itoa(numero, texto, 10);

    UART_string(texto);
}

// =====================================================
// CONTADOR EN LA LCD
// =====================================================

void prepararLineaJuego(void)
{
    lcd_goto_xy(1, 0);

    lcd_write_word("R:    T:00:00");
}

void actualizarContador(uint32_t inicio)
{
    uint32_t segundos;

    uint16_t minutos;

    uint8_t segundosRestantes;

    char texto[6];

    segundos = (tiempoActual() - inicio) / 1000;

    minutos = segundos / 60;

    segundosRestantes = segundos % 60;

    if (minutos > 99)
    {
        minutos = 99;
        segundosRestantes = 59;
    }

    texto[0] = '0' + (minutos / 10);
    texto[1] = '0' + (minutos % 10);
    texto[2] = ':';
    texto[3] = '0' + (segundosRestantes / 10);
    texto[4] = '0' + (segundosRestantes % 10);
    texto[5] = '\0';

    lcd_goto_xy(1, 9);

    lcd_write_word(texto);
}

// =====================================================
// LEER TECLA DURANTE EL JUEGO
// =====================================================

char leerTeclaJuego(uint32_t inicio)
{
    char tecla;

    uint32_t ultimaActualizacion = 0;

    while (1)
    {
        if (tiempoActual() - ultimaActualizacion >= 100)
        {
            actualizarContador(inicio);

            ultimaActualizacion = tiempoActual();
        }

        tecla = buscarTecla();

        if (tecla != 0)
        {
            esperarTiempo(25);

            if (buscarTecla() == tecla)
            {
                while (buscarTecla() != 0)
                {
                    if (tiempoActual() - ultimaActualizacion >= 100)
                    {
                        actualizarContador(inicio);

                        ultimaActualizacion = tiempoActual();
                    }
                }

                return tecla;
            }
        }
    }
}

// =====================================================
// EEPROM - TOP 5
// =====================================================

void iniciarEEPROM(void)
{
    uint8_t i;

    if (EEPROM_read(100) != 0xA5)
    {
        for (i = 0; i < 60; i++)
        {
            EEPROM_update(i, 0xFF);
        }

        EEPROM_update(100, 0xA5);
    }
}

uint8_t obtenerDireccion(uint8_t modo, uint8_t nivel)
{
    return ((modo - 1) * 3 + (nivel - 1)) * 10;
}

uint16_t leerTiempo(uint8_t direccion)
{
    uint16_t bajo;
    uint16_t alto;

    bajo = EEPROM_read(direccion);

    alto = EEPROM_read(direccion + 1);

    return bajo | (alto << 8);
}

void guardarTiempo(uint8_t direccion, uint16_t nuevoTiempo)
{
    uint16_t tiempos[NUM_RECORDS];

    uint8_t i;
    uint8_t j;

    for (i = 0; i < NUM_RECORDS; i++)
    {
        tiempos[i] = leerTiempo(direccion + (i * 2));
    }

    for (i = 0; i < NUM_RECORDS; i++)
    {
        if (nuevoTiempo < tiempos[i])
        {
            for (j = NUM_RECORDS - 1; j > i; j--)
            {
                tiempos[j] = tiempos[j - 1];
            }

            tiempos[i] = nuevoTiempo;

            break;
        }
    }

    for (i = 0; i < NUM_RECORDS; i++)
    {
        EEPROM_update(
            direccion + (i * 2),
            (uint8_t)tiempos[i]
        );

        EEPROM_update(
            direccion + (i * 2) + 1,
            (uint8_t)(tiempos[i] >> 8)
        );
    }
}

// =====================================================
// MOSTRAR RANKING EN TERMINAL
// =====================================================

void mostrarRankingUART(uint8_t direccion, char *nombre)
{
    uint8_t i;

    uint16_t valor;

    UART_string("\r\n");
    UART_string(nombre);
    UART_string("\r\n");

    for (i = 0; i < NUM_RECORDS; i++)
    {
        valor = leerTiempo(direccion + (i * 2));

        UART_string("Lugar ");

        enviarNumero(i + 1);

        UART_string(": ");

        if (valor == 65535)
        {
            UART_string("Sin registro\r\n");
        }
        else
        {
            enviarNumero(valor);

            UART_string(" segundos\r\n");
        }
    }
}

// =====================================================
// LEER OPCION DESDE LA TERMINAL
// =====================================================

char leerOpcionUART(void)
{
    char tecla;

    while (1)
    {
        tecla = UART_read();

        if (tecla >= '0' && tecla <= '9')
        {
            UART_write(tecla);
            UART_string("\r\n");

            return tecla;
        }
    }
}

// =====================================================
// CONSULTAR RANKING DESDE LA TERMINAL
// =====================================================

void consultarRankingSeleccionado(void)
{
    char tecla;

    uint8_t modo;
    uint8_t nivel;
    uint8_t direccion;

    UART_string("\r\n");
    UART_string("============================\r\n");
    UART_string("     PATTERNIX RANKINGS\r\n");
    UART_string("============================\r\n");

    UART_string("SELECCIONA EL MODO DE JUEGO:\r\n");
    UART_string("1 - Secuencias\r\n");
    UART_string("2 - Calculadora\r\n");
    UART_string("Escribe una opcion: ");

    while (1)
    {
        tecla = leerOpcionUART();

        if (tecla == '1')
        {
            modo = 1;
            break;
        }

        if (tecla == '2')
        {
            modo = 2;
            break;
        }

        UART_string("Opcion incorrecta. Escribe 1 o 2: ");
    }

    UART_string("\r\nSELECCIONA LA DIFICULTAD:\r\n");
    UART_string("1 - Facil\r\n");
    UART_string("2 - Medio\r\n");
    UART_string("3 - Dificil\r\n");
    UART_string("Escribe una opcion: ");

    while (1)
    {
        tecla = leerOpcionUART();

        if (tecla == '1')
        {
            nivel = 1;
            break;
        }

        if (tecla == '2')
        {
            nivel = 2;
            break;
        }

        if (tecla == '3')
        {
            nivel = 3;
            break;
        }

        UART_string("Opcion incorrecta. Escribe 1, 2 o 3: ");
    }

    direccion = obtenerDireccion(modo, nivel);

    UART_string("\r\n===== RANKING PATTERNIX =====\r\n");

    if (modo == 1)
    {
        if (nivel == 1)
            mostrarRankingUART(direccion, "Secuencias - Facil");

        else if (nivel == 2)
            mostrarRankingUART(direccion, "Secuencias - Medio");

        else
            mostrarRankingUART(direccion, "Secuencias - Dificil");
    }
    else
    {
        if (nivel == 1)
            mostrarRankingUART(direccion, "Calculadora - Facil");

        else if (nivel == 2)
            mostrarRankingUART(direccion, "Calculadora - Medio");

        else
            mostrarRankingUART(direccion, "Calculadora - Dificil");
    }

    UART_string("==============================\r\n");

    lcd_clear();

    lcd_goto_xy(0, 0);
    lcd_write_word("Ranking enviado");

    lcd_goto_xy(1, 0);
    lcd_write_word("Ver terminal");

    esperarTiempo(2000);
}

// =====================================================
// SELECCIONAR DIFICULTAD CON TECLADO
// =====================================================

uint8_t seleccionarDificultad(void)
{
    char tecla;

    lcd_clear();

    lcd_goto_xy(0, 0);
    lcd_write_word("Dificultad:");

    lcd_goto_xy(1, 0);
    lcd_write_word("1F  2M  3D");

    while (1)
    {
        tecla = leerTecla();

        if (tecla == '1')
            return 1;

        if (tecla == '2')
            return 2;

        if (tecla == '3')
            return 3;

        if (tecla == 'C')
            return 0;
    }
}

// =====================================================
// JUEGO 1 - CREAR SUCESION
// =====================================================

void crearSecuencia(void)
{
    uint8_t i;

    uint16_t base;
    uint16_t paso;

    if (dificultad == 1)
    {
        // Facil: sucesion aritmetica

        base = 1 + rand() % 5;

        paso = 2 + rand() % 4;

        for (i = 0; i < 5; i++)
        {
            sucesion[i] = base + (paso * i);
        }

        respuestaCorrecta = sucesion[4];
    }

    else if (dificultad == 2)
    {
        // Medio: sucesion geometrica

        base = 2 + rand() % 3;

        paso = 2 + rand() % 2;

        sucesion[0] = base;
        sucesion[1] = base * paso;
        sucesion[2] = sucesion[1] * paso;
        sucesion[3] = sucesion[2] * paso;

        respuestaCorrecta = sucesion[3] * paso;
    }

    else
    {
        // Dificil: sucesion cuadratica
        // Base aleatoria del 1 al 10

        base = 1 + rand() % 10;

        for (i = 0; i < 5; i++)
        {
            sucesion[i] = base * (i + 1) * (i + 2);
        }

        respuestaCorrecta = sucesion[4];
    }
}

// =====================================================
// MOSTRAR SUCESION
// =====================================================

void mostrarSecuencia(void)
{
    uint8_t i;

    lcd_clear();

    lcd_goto_xy(0, 0);

    for (i = 0; i < 4; i++)
    {
        mostrarNumero(sucesion[i]);

        lcd_write_character(' ');
    }

    lcd_write_character('?');

    prepararLineaJuego();
}

// =====================================================
// LEER RESPUESTA DEL JUEGO
// =====================================================

uint16_t leerRespuestaJuego(uint32_t inicio)
{
    char tecla;

    uint16_t numero = 0;

    uint8_t posicion = 0;

    while (1)
    {
        tecla = leerTeclaJuego(inicio);

        if (tecla >= '0' && tecla <= '9')
        {
            if (posicion < 4)
            {
                numero = numero * 10 + (tecla - '0');

                posicion++;

                lcd_goto_xy(1, 2);

                mostrarNumero(numero);
            }
        }

        else if (tecla == 'C')
        {
            numero = 0;

            posicion = 0;

            lcd_goto_xy(1, 0);
            lcd_write_word("R:    T:00:00");

            actualizarContador(inicio);
        }

        else if (tecla == '#')
        {
            if (posicion > 0)
            {
                return numero;
            }
        }
    }
}

// =====================================================
// JUEGO 1 - SUCESIONES NUMERICAS
// =====================================================

void juegoSecuencia(void)
{
    uint32_t inicio;
    uint32_t final;

    uint16_t segundos;

    uint16_t respuesta;

    uint8_t direccion;

    dificultad = seleccionarDificultad();

    if (dificultad == 0)
        return;

    // Generar una semilla para los numeros aleatorios

    srand((unsigned int)tiempoActual());

    direccion = obtenerDireccion(1, dificultad);

    crearSecuencia();

    mostrarSecuencia();

    UART_string("\r\nJUEGO DE SUCESIONES\r\n");
    UART_string("Encuentra el siguiente numero.\r\n");

    inicio = tiempoActual();

    while (1)
    {
        respuesta = leerRespuestaJuego(inicio);

        if (respuesta == respuestaCorrecta)
        {
            break;
        }

        beepError();

        UART_string("Respuesta incorrecta. Intenta nuevamente.\r\n");

        lcd_goto_xy(1, 0);
        lcd_write_word("R:    T:00:00");

        actualizarContador(inicio);
    }

    final = tiempoActual();

    segundos = (final - inicio) / 1000;

    if (segundos == 0)
        segundos = 1;

    guardarTiempo(direccion, segundos);

    beep();

    lcd_clear();

    lcd_goto_xy(0, 0);
    lcd_write_word("Correcto!");

    lcd_goto_xy(1, 0);
    lcd_write_word("Tiempo: ");

    mostrarNumero(segundos);

    lcd_write_word(" s");

    UART_string("Secuencia correcta.\r\n");
    UART_string("Tiempo: ");

    enviarNumero(segundos);

    UART_string(" segundos\r\n");

    esperarTiempo(2000);
}

// =====================================================
// JUEGO 2 - CREAR OPERACION
// =====================================================


void crearOperacion(void)
{
	uint8_t tipo;
	uint8_t exponente;
	uint8_t i;

	if (dificultad == 1)
	{
		numero1 = 1 + rand() % 9;
		numero2 = 1 + rand() % 9;

		if (rand() % 2 == 0)
		{
			operacion = '+';
			resultado = numero1 + numero2;
		}
		else
		{
			operacion = '-';

			if (numero1 < numero2)
			{
				uint16_t temporal = numero1;
				numero1 = numero2;
				numero2 = temporal;
			}

			resultado = numero1 - numero2;
		}
	}
	else if (dificultad == 2)
	{
		numero1 = 1 + rand() % 9;
		numero2 = 1 + rand() % 9;

		if (rand() % 2 == 0)
		{
			operacion = '*';
			resultado = numero1 * numero2;
		}
		else
		{
			operacion = '/';

			resultado = numero1;
			numero1 = numero1 * numero2;
		}
	}
	else
	{
		// Dificil: multiplicacion, division y potencia
		tipo = rand() % 3;

		if (tipo == 0)
		{
			// Multiplicacion de dos cifras
			numero1 = 10 + rand() % 90;
			numero2 = 10 + rand() % 90;

			operacion = '*';

			resultado = numero1 * numero2;
		}
		else if (tipo == 1)
		{
			// Division exacta con divisor de dos cifras
			numero2 = 10 + rand() % 21;

			resultado = 2 + rand() % 29;

			numero1 = numero2 * resultado;

			operacion = '/';
		}
		else
		{
			// Potencias
			numero1 = 2 + rand() % 8;

			exponente = 2 + rand() % 3;

			numero2 = exponente;

			operacion = '^';

			resultado = 1;

			for (i = 0; i < exponente; i++)
			{
				resultado *= numero1;
			}
		}
	}
}


// =====================================================
// MOSTRAR OPERACION
// =====================================================

void mostrarOperacion(void)
{
    lcd_clear();

    lcd_goto_xy(0, 0);

    mostrarNumero(numero1);

    lcd_write_character(operacion);

    mostrarNumero(numero2);

    lcd_write_character('=');

    prepararLineaJuego();
}

// =====================================================
// LEER RESPUESTA DE CALCULADORA
// =====================================================

uint16_t leerRespuestaCalculadora(uint32_t inicio)
{
    char tecla;

    uint16_t numero = 0;

    uint8_t posicion = 0;

    while (1)
    {
        tecla = leerTeclaJuego(inicio);

        if (tecla >= '0' && tecla <= '9')
        {
            if (posicion < 4)
            {
                numero = numero * 10 + (tecla - '0');

                posicion++;

                lcd_goto_xy(1, 2);

                mostrarNumero(numero);
            }
        }

        else if (tecla == 'C')
        {
            numero = 0;

            posicion = 0;

            lcd_goto_xy(1, 0);
            lcd_write_word("R:    T:00:00");

            actualizarContador(inicio);
        }

        else if (tecla == '#')
        {
            if (posicion > 0)
            {
                return numero;
            }
        }
    }
}

// =====================================================
// JUEGO 2 - CALCULADORA
// =====================================================

void juegoCalculadora(void)
{
    uint8_t pregunta;

    uint16_t respuesta;

    uint32_t inicio;
    uint32_t final;

    uint16_t segundos;

    uint8_t direccion;

    dificultad = seleccionarDificultad();

    if (dificultad == 0)
        return;

    srand((unsigned int)tiempoActual());

    direccion = obtenerDireccion(2, dificultad);

    UART_string("\r\nJUEGO DE CALCULADORA\r\n");

    inicio = tiempoActual();

    for (pregunta = 0; pregunta < NUM_PREGUNTAS; pregunta++)
    {
        crearOperacion();

        mostrarOperacion();

        while (1)
        {
            respuesta = leerRespuestaCalculadora(inicio);

            if (respuesta == resultado)
            {
                beep();

                UART_string("Respuesta correcta.\r\n");

                break;
            }
            else
            {
                beepError();

                UART_string("Respuesta incorrecta. Intenta nuevamente.\r\n");

                lcd_goto_xy(1, 0);
                lcd_write_word("R:    T:00:00");

                actualizarContador(inicio);
            }
        }
    }

    final = tiempoActual();

    segundos = (final - inicio) / 1000;

    if (segundos == 0)
        segundos = 1;

    guardarTiempo(direccion, segundos);

    lcd_clear();

    lcd_goto_xy(0, 0);
    lcd_write_word("Juego terminado");

    lcd_goto_xy(1, 0);
    lcd_write_word("Tiempo: ");

    mostrarNumero(segundos);

    lcd_write_word(" s");

    UART_string("Calculadora terminada.\r\n");
    UART_string("Tiempo: ");

    enviarNumero(segundos);

    UART_string(" segundos\r\n");

    beep();

    esperarTiempo(2000);
}

// =====================================================
// MENU PRINCIPAL
// =====================================================

void mostrarMenu(void)
{
    lcd_clear();

    lcd_goto_xy(0, 0);
    lcd_write_word("1 Secuencias");

    lcd_goto_xy(1, 0);
    lcd_write_word("2 Calculadora");
}

// =====================================================
// PANTALLA DE INICIO
// =====================================================

void pantallaInicio(void)
{
    lcd_clear();

    lcd_goto_xy(0, 0);
    lcd_write_word("PATTERNIX");

    lcd_goto_xy(1, 0);
    lcd_write_word("Bienvenido");

    esperarTiempo(2000);
}

// =====================================================
// MAIN
// =====================================================

int main(void)
{
    char tecla;

    // Buzzer

    DDRB |= (1 << BUZZER);

    PORTB &= ~(1 << BUZZER);

    // UART

    DDRD &= ~(1 << PD0);

    DDRD |= (1 << PD1);

    UART_init();

    // Teclado

    teclado_init();

    // LCD

    lcd_init();

    // EEPROM

    iniciarEEPROM();

    // Timer1

    iniciarTimer();

    // Activar interrupciones

    sei();

    UART_string("\r\n====================\r\n");
    UART_string("      PATTERNIX\r\n");
    UART_string("====================\r\n");
    UART_string("1 = Secuencias\r\n");
    UART_string("2 = Calculadora\r\n");
    UART_string("A = Consultar ranking\r\n");

    pantallaInicio();

    while (1)
    {
        mostrarMenu();

        tecla = leerTecla();

        if (tecla == '1')
        {
            beep();

            juego = 1;

            juegoSecuencia();
        }

        else if (tecla == '2')
        {
            beep();

            juego = 2;

            juegoCalculadora();
        }

        else if (tecla == 'A')
        {
            beep();

            consultarRankingSeleccionado();
        }
    }
}
