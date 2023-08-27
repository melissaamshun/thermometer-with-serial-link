/*
	Encoder routines and interrupt
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "encoder.h"

ISR(PCINT2_vect);
unsigned char a, b;
unsigned char cVal;
unsigned char new_state, old_state;

void encoder_init(void)
{
    DDRD &= ~((1 << PD3) | (1 << PD2));
    PORTD |= ((1 << PD3) | (1 << PD2));

	PCMSK2 |= ((1 << PCINT19) | (1 << PCINT18));

	cVal = (PIND & ((1 << PD4)|(1 << PD5)));  		// Read the input bits and determine A and B.
	a = (cVal & (1 << PD4)); 
	b = (cVal & (1 << PD5));	  
	
    if (!b && !a) //00
	old_state = 0;
    else if (!b && a) //01
	old_state = 1;
    else if (b && !a) //10
	old_state = 3;
    else if (b && a)  //11
	old_state = 2;
	
    new_state = old_state;
	count_change = 0;

	int eeprom_int = (eeprom_read_byte((void *) 100)) - '0';
    int eeprom_dec = (eeprom_read_byte((void *) 200)) - '0';

    if(eeprom_int <= 90)
    {
        if(eeprom_int >= 50){encodert = (eeprom_int * 10) + eeprom_dec;}
		else{encodert = 780;}
    }
	else{encodert = 780;}

}

ISR(PCINT2_vect)
{
	cVal = (PIND & ((1 << PD3)|(1 << PD2)));  		// Read the input bits and determine A and B.
	a = (cVal & (1 << PD2)); 
	b = (cVal & (1 << PD3));	
	
	if (old_state == 0) //00
	{
    	// Handle A and B inputs for state 0
		if(!b && a) //01
		{
			new_state = 1;
			encodert += 1;
		}
		else if(b && !a) //10
		{
			new_state = 3;
			encodert -= 1;
		}

	}
	else if (old_state == 1) //01
	{
    	// Handle A and B inputs for state 1
		if(!b && !a) //00
		{
			new_state = 0;
			encodert -= 1;
		}
		else if(b && a) //11
		{
			new_state = 2;
			encodert += 1;
		}
	}
	else if (old_state == 2) //11
	{
    	// Handle A and B inputs for state 2
		if(b && !a) //10
		{
			new_state = 3;
			encodert += 1;
		}
		else if(!b && a) //01
		{
			new_state = 1;
			encodert -= 1;
		}
	} 
	else	// old_state = 3; 10
	{   
		// Handle A and B inputs for state 3
		if(b && a) //11
		{
			new_state = 2;
			encodert -= 1;
		}
		else if(!b && !a) //00
		{
			new_state = 0;
			encodert += 1;
		}
	}
	// If state count_change, update the value of old_state,
	// and set a flag that the state has ount_change.

	if(encodert > 900){encodert = 900;} 
	else if(encodert < 500){encodert = 500;}

	if (new_state != old_state) 
	{
    	count_change = 1;	
		old_state = new_state;
		eeprom_update_byte((void *) 100, ((encodert / 10) + '0'));
    	eeprom_update_byte((void *) 200, ((encodert % 10) + '0'));
	}



}