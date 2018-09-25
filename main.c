/*
 * finalfinalfinal project.c
 *
 * Created: 2018-04-15 12:59:50 PM
 * Author : nxwin
 * simon says game device. LED flashes as prompts for the player to replicate by pressing a corresponding
 * button. Order of sequence is compared to the players input after each round. Every increment
 * in round increases the length of the LED pattern displayed, starting from 3 flashes. 
 * 
 */ 

#define F_CPU 1000000
#include "defines.h"
#include <avr/io.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#include <stdint.h>

#include <stdio.h>

#include <util/delay.h>
#include <ctype.h>
#include "lcd.h"
#include "hd44780.h"

FILE lcd_str = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE);

#define LEDg6 5 //PINC5
#define LEDg5 4 //PINC4 
#define LEDr4 5 //PINB5
#define LEDr3 4 //PINB4 
#define LEDr2 0 //PIND0 
#define LEDy1 1 //PIND1
#define BTN1 6 //PINB6
#define BTN2 7 //PINB7
#define BTN3 5 //PIND5
#define BTN4 6 //PIND6
#define BTN5 7 //PIND7
#define BTN6 0 //PINB0 
#define NORMAL 800
#define HARD 300 
#define MAXROUNDNUM 10 
#define MAXLEDNUM 6 
#define ONESEC 1000
#define everybtn ((PINB & ((1<<BTN1)|(1<<BTN2)|(1<<BTN6))) && (PIND & ((1<<BTN3)|(1<<BTN4)|(1<<BTN5))))

int SequenceLength = 3;
int modechoice = 0; 
int current_round = 1;
int sequence_array[MAXROUNDNUM]; //used to store sequence order
int failedgame = 0; 
int ButtonPressed[MAXROUNDNUM]; //compares buttons pressed to sequence array.

void peripheral_inti(void);
int select_difficulty(void);
int randomLEDorder(void); 
void first_sequence(int); 
void incre_sequence(void); 
void game_phase(void); 
void toggleLED(int, int); 
void allLEDoff(void); 
void countdown(void); 
void printlcd(char*); 
void endofgame(void);


int main(void)
{
    peripheral_inti(); 
	lcd_init();
	 
	stderr = &lcd_str;
	hd44780_outcmd('\00');
	fprintf(stderr, "White btn:normal                        Black btn:hard");
	
	modechoice = select_difficulty();
	srand(time(NULL));
	
    while (1) 
    {
		countdown();
		first_sequence(current_round); 
		game_phase();
		incre_sequence(); //move this or use more conditions
		endofgame(); 
    }
}

void peripheral_inti (void)
{
	//initialize the LEDS
	allLEDoff();
	DDRD |= (1<<LEDr2)|(1<<LEDy1); 
	DDRC |= (1<<LEDg6)|(1<<LEDg5);
	DDRB |= (1<<LEDr4)|(1<<LEDr3);
		
	//initialize the buttons
	PORTB |= (1<<BTN1)|(1<<BTN2)|(1<<BTN6); 
	PORTD |= (1<<BTN3)|(1<<BTN4)|(1<<BTN5); 
	DDRB &= ~((1<<BTN1)|(1<<BTN2)|(1<<BTN6));
	DDRD &= ~((1<<BTN3)|(1<<BTN4)|(1<<BTN5));
}

int select_difficulty (void)
{
	/*the code in segment was made unnaturally longer to solve an issue where some button presses never registered
	or a button would proc the incorrect result. 
	eg. button 4 and 5 would proc nothing; button 1 and 2 would alternate between triggering NORMAL or HARD*/ 
	int value; 
	while(1) 
	{
		if ( (((1<<BTN1)|(1<<BTN2)) != (PINB & ((1<<BTN1)|(1<<BTN2)))) )
		{
			while((((1<<BTN1)|(1<<BTN2)) != (PINB & ((1<<BTN1)|(1<<BTN2)))) ); //Debouncing
			value = NORMAL;
			PORTD |= (1<<LEDy1);
			return value;
		}
		
		if (((1<<BTN3) != (PIND & (1<<BTN3)) ))
		{
			while (((1<<BTN3) != (PIND & (1<<BTN3)) )); 
			value = NORMAL;
			PORTD |= (1<<LEDy1);
			return value;
		}

		if (  ((1<<BTN4)|(1<<BTN5)) != (PIND & ((1<<BTN4)|(1<<BTN5)) ) )
		{
			while ( ( ((1<<BTN4)|(1<<BTN5)) != (PIND & ((1<<BTN4)|(1<<BTN5)))) ); //Debouncing
			value = HARD;
			PORTD |= (1<<LEDr2); 
			return value;
		}
		
		if ((1<<BTN6) != (PINB & (1<<BTN6)))
		{
			while( (1<<BTN6) != (PINB & (1<<BTN6)) );
			value = HARD;
			PORTD |= (1<<LEDr2);
			return value;
		} 
	} 
}

void countdown(void)
{
	if (current_round != 1)
	{
		return;
	}
	
	//print out countdown
	stderr = &lcd_str;  
	
	hd44780_outcmd('\00');
	fprintf(stderr, "Start in 3");                                                 
	_delay_ms(1000); 
	hd44780_outcmd('\00');
	fprintf(stderr, "Start in 2");
	_delay_ms(1000);
	allLEDoff(); 
	
	hd44780_outcmd('\00');
	fprintf(stderr, "Start in 1");
	_delay_ms(1000);

}


void first_sequence (int stagenum)
{
	int i; 
	if (current_round != 1)
	{
		return; 
	}
	
	for (i = 0; i < (current_round + 2); i++)
	{
		sequence_array[i] =  randomLEDorder();
	}
	//positions 0,1,2 in the array are filled
	//next need to set those positions as high to output to shift register
	
	int y;
	int ledspot;
	for (y = 0; y < (current_round + 2); y++)
	{
		ledspot = sequence_array[y];
		toggleLED(modechoice, ledspot);
	}
	return;
}

void incre_sequence (void)
{
	if (failedgame == 1)
	{
		return; 
	}
	
	sequence_array[current_round + 2] = randomLEDorder();
	//starts from position 3 of array (a 4th led flash)
	int z;
	int ledspot;
	for (z = 0; z < (current_round + 3); z++)
	{
		ledspot = sequence_array[z];	//re-flashes the previous sequence
		toggleLED(modechoice, ledspot); //send in LED number to flash 
	}
	SequenceLength++; 
}

int randomLEDorder (void)
{
	int result;
	result = rand() % MAXLEDNUM + 1; 	//generate a random value from 1 to 6
	return result;
}

void toggleLED (int duration, int position)
{
	//position is the direct LED number from the pattern array
	//flash LED for a small duration 
	switch (position)
	{
		case 1: 
		PORTD |= (1<<LEDy1);
		break; 
		case 2: 
		PORTD |= (1<<LEDr2); 
		break;
		case 3:
		PORTB |= (1<<LEDr3);
		break; 
		
		case 4: 
		PORTC |= (1<<LEDr4);
		break;
		
		case 5: 
		PORTC |= (1<<LEDg5);
		break;
		case 6: 
		PORTB |= (1<<LEDg6);
		break;
		
	}
	
	switch (duration)
	{
		case NORMAL:
		_delay_ms(800);
		break;
		case HARD:
		_delay_ms(300);
		break;
	}
	allLEDoff(); 
}

void game_phase (void)
{
	//function for where player input is compared to the displayed pattern
	
	int LEDOrder = 0;
	int x = 0;
		while(LEDOrder < SequenceLength) 
		//This loop will check which button is pressed and compare it with the button that should’ve been pressed.
		{
			if ((1<<BTN1) != (PINB & (1<<BTN1))) 
			{	
				_delay_ms(50);
				while((1<<BTN1) != (PINB & (1<<BTN1))); //Debouncing solution
				ButtonPressed[x] = 1;
			
				
				x = x + 1;
				LEDOrder = LEDOrder + 1;
				
			}
			
			else if ((1<<BTN2) != (PINB & (1<<BTN2))) 
			{	
				_delay_ms(50);
				while((1<<BTN2) != (PINB & (1<<BTN2))); //Debouncing 
				ButtonPressed[x] = 2;
				
			
				x = x + 1;
				LEDOrder = LEDOrder + 1;
				
			}

			else if ((1<<BTN3) != (PIND & (1<<BTN3))) 
			{	
				_delay_ms(50);
				while((1<<BTN3) != (PIND & (1<<BTN3))); //Debounce
				ButtonPressed[x] = 3;

			
				x = x + 1;
				LEDOrder = LEDOrder + 1;
				
			}
			
			else if ((1<<BTN4) != (PIND & (1<<BTN4))) 
			{			
				_delay_ms(50);	
				while((1<<BTN4) != (PORTD & (1<<BTN4))); //Debouncing solution
				ButtonPressed[x] = 4;

				
				x = x + 1;
				LEDOrder = LEDOrder + 1;
				
			}

			else if ((1<<BTN5) != (PIND & (1<<BTN5))) 
			{
				_delay_ms(50);
				while((1<<BTN5) != (PIND & (1<<BTN5))); //Debouncing solution
				ButtonPressed[x] = 5;

				
				x = x + 1;
				LEDOrder = LEDOrder + 1;
				
			}

			else if ((1<<BTN6) != (PINB & (1<<BTN6))) 
			{
				_delay_ms(50);
				while((1<<BTN6) != (PINB & (1<<BTN6))); //Debouncing solution
				ButtonPressed[x] = 6;

				
				x = x + 1;
				LEDOrder = LEDOrder + 1;
				
			}	 
		}
		
		
		int i;
		for (i = 0; i < SequenceLength; i++)
		 {	
			int a,b; 
			a = ButtonPressed[i]; 
			b = sequence_array[i];
			if (a != b)
			{
			failedgame = 1;
			return;
			}
		}
		
		
		current_round++; 
		
	  return;
}

void allLEDoff (void)
{
	PORTD &= ~((1<<LEDr2)|(1<<LEDy1));
	PORTC &= ~((1<<LEDg6)|(1<<LEDg5));
	PORTB &= ~((1<<LEDr4)|(1<<LEDr3));
}

void endofgame (void)
{
	
	if ( (failedgame == 1) || (current_round == 10) )
	{	
		//print out results
		//fprintf ("Rounds won: %u                 Play again?", current_round)
		stderr = &lcd_str;
		hd44780_outcmd('\00');
		fprintf(stderr, "Rounds won: %u", (current_round - 1));
		
		
		hd44780_outcmd('\00');
		int p; 
		for(p=0; p<3; p++)
		{

		fprintf(stderr, "%u%u " , sequence_array[p], ButtonPressed[p]);
		}
		
		while(1);
	
	}
	else 
	{
		return; 
	}

}

