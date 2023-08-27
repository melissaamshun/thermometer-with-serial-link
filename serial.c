/*
	Serial routines and button_change interrupt
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdio.h>

#include "serial.h"

#define FOSC 16000000 // Clock frequency
#define BAUD 9600 // Baud rate used
#define MYUBRR (FOSC/16/BAUD-1) // Value for UBRR0

ISR(PCINT1_vect);
ISR(USART_RX_vect);

unsigned char butVal;
unsigned char l, r;

unsigned char serial_state = 0;
char received_data;
char buf[5];
int buf_cnt = 0;


void serial_init()
{
    //button_change and tri-state registers
    DDRC &= ~((1 << PC2) | (1 << PC1));
    PORTC |= ((1 << PC2) | (1 << PC1));
    DDRC |= (1 << PC4);
    PORTC &= ~(1 << PC4);

	//enable interrupts
	PCMSK1 |= ((1 << PCINT10) | (1 << PCINT9));
    UCSR0B |= (1 << RXCIE0);

    UBRR0 = MYUBRR; // Set baud rate
    UCSR0B |= (1 << TXEN0 | 1 << RXEN0); // Enable RX and TX
    UCSR0C = (3 << UCSZ00); // Async., no parity, 1 stop bit, 8 data bits

    button_change = 1;
    serial_flag = 0;

}

char rx_char()
{
    // Wait for receive complete flag to go high
    while ( !(UCSR0A & (1 << RXC0)) ) {}
    return UDR0;
}

void tx_char(char ch)
{
    // Wait for transmitter data register empty
    while ((UCSR0A & (1<<UDRE0)) == 0) {}
    UDR0 = ch;
}

ISR(PCINT1_vect) //isr for button_changes
{  
    butVal = PINC;
    r = (butVal & (1 << PC2));
    l = (butVal & (1 << PC1));
    if(!l && r)
    { 
        button_change = 1;
    }
    if(l && !r)
    {
        button_change = 2;
    } 
}

ISR(USART_RX_vect) //isr for rx
{
    received_data = rx_char();
    if(serial_state == 0)
    {
        buf_cnt = 0;
        if(received_data == '@'){serial_state = 1;}
        buf[0] = 0;
        buf[1] = 0;
        buf[2] = 0;

    }
    else if(serial_state == 1)
    {
        if((received_data == '+') || (received_data == '-')){serial_state = 2;}

        else{serial_state = 0;}
    }
    else if(serial_state == 2)
    {
        if((buf_cnt < 3) && (received_data >= '0') && (received_data <= '9'))
        {
            buf[buf_cnt] = received_data;
            buf_cnt += 1;
        }
        else if(received_data == '#'){serial_state = 3;}
        else if(buf_cnt > 3){serial_state = 0;}
    }
    else if(serial_state == 3)
    {
        if(buf_cnt > 1)
        {
            serial_state = 0;
            serial_flag = 1;
            sscanf(buf,  "%d", &serialt);
        }
        else{serial_state = 0;}
        
    }
}