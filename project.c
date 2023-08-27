/********************************************
 *
 *  Name:	Melissa Shun
 *  Email:	mshun@usc.edu
 *  Section: Wed 2pm
 *  Assignment: Project
 *
 ********************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "ds18b20.h"
#include "lcd.h"
#include "encoder.h"
#include "serial.h"

void timer2_init(void);
void timer1_init(void);
void timer0_init(void);

volatile char temp_flag = 0;
volatile char buzz_count = 0;

int main(void)
{
    
    //ds things
    unsigned char dsVal[2];
    unsigned int temp = 0;
    int fahrt;
    int fahr_int;
    int fahr_dec;
    char temp_change = 1;
    char fahr_string[17];
    char *temp_status = "OK";
    char update_lcd = 1;

    //encoder things
    int encoder_int;
    int encoder_dec;
    char encoder_string[17];

    //serial comm things
    char tx[7];
    int i = 0;

    //button things
    char *local_source = ">";
    char *remote_source = " ";

    //Set LEDs, servo, buzzer register as outputs and enable interrupts
    DDRB |= ((1 << PB5) | (1 << PB4) | (1 << PB3)); 
    DDRC |= (1 << PC5); 
    PCICR |= ((1 << PCIE2) | (1 << PCIE1));	
    sei();
    
    //initializing functions for all ds, lcd, encoder, and serial registers
    ds_init();
    lcd_init();
    encoder_init();
    serial_init();
    
    //timer initializing
    timer2_init();
    timer1_init();
    timer0_init();

    //code for splash screen
    lcd_writecommand(1); 
    lcd_moveto(0, 2);
    lcd_stringout("Melissa Shun");           
    lcd_moveto(1, 1);
    lcd_stringout("EE 109 Project"); 
    _delay_ms(1000);
    lcd_writecommand(1);
    
    ds_convert();
    encoder_int = encodert / 10;
    encoder_dec = encodert % 10;
    temp = ((dsVal[1] << 8) + dsVal[0]);
    fahrt = (((temp * 90) / 5) / 16) + (320);
    fahr_int = fahrt / 10;
    fahr_dec = fahrt % 10;
    ds_convert();

    snprintf(fahr_string, 17, "%sLC:%d.%d%sRM:%d", local_source, fahr_int, fahr_dec, remote_source, serialt);    lcd_moveto(0,0);
    lcd_stringout(fahr_string);
    snprintf(encoder_string, 17, "TEST: %d.%d", encoder_int, encoder_dec);
    lcd_moveto(1,0);
    lcd_stringout(encoder_string);

    while(1)
    {
       
        //updates the encoder
        if(count_change == 1)
        {   
            count_change = 0;                          //from isr in encoder.c
            encoder_int = encodert / 10;               //separate integer from decimal val
            encoder_dec = encodert % 10;
            update_lcd = 1;
        }

        //updates local temperature
        temp_change = ds_temp(dsVal);    
        if(temp_change == 1)
        {   
            temp_change = 0;                           //from isr in ds18b.c
            temp = ((dsVal[1] << 8) + dsVal[0]);
            fahrt = (((temp * 90) / 5) / 16) + (320);  //convert from C to F
            fahr_int = fahrt / 10;                     //separate integer from decimal val
            fahr_dec = fahrt % 10;

            ds_convert();

            //transfers data to remote device
            i = 0;              
            snprintf(tx, 7, "@+%d#", fahr_int);
            while(tx[i] != '\0')
            {
                tx_char(tx[i]);
                i += 1;
            }
                
            update_lcd = 1;
        } 

        //updates data from serial comm
        if(serial_flag == 1){update_lcd = 1;}

        //updates servo from local/remote data, as well as the indicator ">" on lcd
        if(button_change == 1) 
        {
            OCR2A = 52 + (((fahrt * -2) / 5) / 10);
            local_source = ">";
            remote_source = " ";
        }
        if(button_change == 2)
        {
            OCR2A = 52 + (((serialt * -20) / 5) / 10);  
            local_source = " ";
            remote_source = ">";
        }     

        //updates LCD
        if(update_lcd == 1)
        {
            update_lcd = 0;

            //compares ds and encoder values, updates LED and buzzer appropriately
            if(fahrt > encodert)
            {
                if(fahrt >= (encodert + 30))
                {
                    temp_status = "HOT ";
                    temp_flag = 2;
                }
                else
                {
                    temp_status = "WARM";
                    temp_flag = 1;
                    buzz_count = 0;
                }
            }
            else
            {
                temp_status = "OK  ";
                temp_flag = 0;
                buzz_count = 0;
            }   

            //updates LCD
            snprintf(fahr_string, 17, "%sLC:%d.%d%sRM:%d", local_source, fahr_int, fahr_dec, remote_source, serialt);
            lcd_moveto(0,0);
            lcd_stringout(fahr_string);
            snprintf(encoder_string, 17, "%d.%d %s", encoder_int, encoder_dec, temp_status);
            lcd_moveto(1,6);
            lcd_stringout(encoder_string);
            
        }
         
    }
    return 0;
}

void timer2_init(void) //timer for servo
{
    TCCR2A |= (0b11 << WGM20);  // Fast PWM mode, modulus = 256
    TCCR2A |= (0b10 << COM2A0); // Turn D11 on at 0x00 and off at OCR2A
    OCR2A = 24;  
    OCR2B = 128;              // Initial pulse duty cycle of 50%
    TCCR2B |= (0b111 << CS20);  // Prescaler = 1024 for 16ms period
}

void timer1_init(void) //timer for LEDs
{
	TCCR1B |= (1 << WGM12);
    TIMSK1 |= (1 << OCIE1A);
    OCR1A = 30000;
    TCCR1B |= (1 << CS12);   
}

void timer0_init(void) //timer for buzzer
{
	TCCR0B |= (1 << WGM02);
    TIMSK0 |= (1 << OCIE0A);
    OCR0A = 30000;
    TCCR0B |= (1 << CS02);   
}

ISR(TIMER1_COMPA_vect) //LED isr
{
    if(temp_flag == 0)
    {
        PORTB |= (1 << 5);
        PORTB &= ~(1 << 4);
    }

    if(temp_flag == 1)
    {
        PORTB &= ~(1 << 5);
        PORTB ^= (1 << 4); //for blinking
    }

    if(temp_flag == 2)
    {
        PORTB &= ~(1 << 5);
        PORTB |= (1 << 4);   
    }
}

ISR(TIMER0_COMPA_vect) //buzzer isr
{
    if(temp_flag == 2)
    {
        if(buzz_count < 100)
        {
            PORTC ^= (1 << 5);
            buzz_count += 1;
        }

        else if(buzz_count >= 100)
        {
            PORTC &= ~(1 << 5);
        }
    }
}