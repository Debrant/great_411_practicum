/*************************************************************************
 *  	J.S.PEIRCE
 * 	AVR MULTIPLE INPUT, CORRESPONDING OUTPUT TEST PROGRAM
 * 	(INTERRUPT BASED)
 * 	ECE411 - FALL 2014
 * 	TEAM T01
 * 	30 OCTOBER 2014
 *
 * 	@TOOLCHAIN:
 * 		AVR-GCC 
 * 		AVR-GDB
 * 		AVR-LIBC
 * 	@BOARD:
 * 		AVRDRAGON BOARD via USB (ISP CONFIGURATION)
 * @USAGE:
 * 1) COMPILE  WITH:
 * avr-gcc -mmcu=atmega328 -Wall -Os -std=gnu99 -o AVR_INTS2.elf AVR_INTS2.c
 * 
 * 2) LINK WITH:
 * avr-objcopy -j .text -j .data -O ihex AVR_INTS2.elf AVR_INTS2.hex
 *
 * 3) CONNECT AVRDRAGON...
 *
 * -MUST RUN BURN SCRIPT (burn) AS SUDO OR ROOT!:
 * -add directory to PATH or from directory containing ALL files type:
 * 
 * 4) sudo ./burn
 * -you will be prompted for passwd if not root, enter it...
 * -script will prompt for part, use: m328
 * -script will prompt for .HEX file, use: AVR_INT2.hex
 * -script will confirm args, then run if correct
 *
 ************************************************************************
 ***************CHANGELOG:***********************************************
 ************************************************************************
 *@27 OCTOBER 2014:
 * ADDED INPUT SUPPORT 
 *@29 OCTOBER 2014:
 * ADDED INTERRUPT SUPPORT
 *@30 OCTOBER 2014:
 * BUILT CODE FOR MULTIPLE INTERRUPT INPUT PATTERNS WHICH THEN
 *  CORRESPOND TO OUTPUT COMBINATIONS OF LEDS
 * UPDATED COMPILER FLAGS TO USE: '-std=gnu99' (NEEDED FOR USE OF ATOMIC
 *  BLOCK SYNTAX).
 * UPDATED COMPILER FLAGS, HAD: 'obj-dump' INSTEAD OF: 'obj-copy' SORRY!! 
 * TODO: BUILD A D*MN Makefile.....
 * TODO: ADD PWM OUTPUT PATTERNS, ADJUST LOGIC TO BE LESS HACKISH...
 * TODO: GET/MAKE A HALLOWEEN COSTUME!...
 * **********************************************************************/
#define F_CPU 1000000UL

#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include<stdint.h>
#include<stdbool.h>
#include<util/atomic.h>

#define INPUT_PORT PORTD
#define OUTPUT_PORT PORTB

/********PORT DEFINITION AND LAYOUT:***************************************
PINx --> PORT x INPUT PINS REGISTER, STORES CURRENT STATE OF PINS:
_________________________________________________________________________
|__PINx0__|__PINx1_|__PINx2_|__PINx3_|__PINx4_|__PINx5_|__PINx6_|__PINx7_|

DDRx --> PORT x DATA DIRECTION REGISTER, CONFIGURE AS INPUT OR OUTPUT:
_________________________________________________________________________
|__DDRx0__|__DDRx1_|__DDRx2_|__DDRx3_|__DDRx4_|__DDRx5_|__DDRx6_|__DDRx7_|

PORTx --> PORT x DATA REGISTER, CURRENT OUTPUT VALUE OF PORT PINS:
_________________________________________________________________________
|_ PORTx0_|_PORTx1_|_PORTx2_|_PORTx3_|_PORTx4_|_PORTx5_|_PORTx6_|_PORTx7_|

EICRA --> EXTERNAL INTERRUPT CONTROL REGISTER A:
_________________________________________________________________________
|___RES__|___RES___|___RES___|___RES___|__ISC11_|_ISC10_|_ISC01_|_ISC00__|

EIMSK --> EXTERNAL INTERRRUPT MASK CONTROLLER:
_________________________________________________________________________
|___RES__|___RES___|___RES___|___RES___|__RES___|___RES___|_INT1_|_INT0__|

EIFR  --> EXTERNAL INTERRUPT FLAG REGISTER:
_________________________________________________________________________
|___RES__|___RES___|___RES___|___RES___|__RES___|__RES__|_INTF1_|_INTF0__|

PCICR --> PIN CHANGE INTERRUPT CONTROLLER:
_________________________________________________________________________
|_ PORTx0_|_PORTx1_|_PORTx2_|_PORTx3_|_PORTx4_|_PORTx5_|_PORTx6_|_PORTx7_|

PCIFR --> PIN CHANGE INTERRUPT FLAG REGISTER:
_________________________________________________________________________
|_ PORTx0_|_PORTx1_|_PORTx2_|_PORTx3_|_PORTx4_|_PORTx5_|_PORTx6_|_PORTx7_|

DESIRED FREQUENCIES: (TONES)
G4: 384.87 -> 2.598 mSec
A4: 432.00 -> 2.315 mSec
B4: 484.90 -> 2.062 mSec
C5: 513.74 -> 1.947 mSec
D5: 576.65 -> 1.734 mSec

*************************************************************************/
/*USE VOLATILE SO COMPILER DOES NOT OPTIMIZE OUT*/
volatile bool update = false;
volatile uint8_t temp_out;

volatile struct input {
	volatile uint8_t prior_state;
	volatile uint8_t PRESCALER;//TODO: ADD USE OF PRESCALER FOR PWM OUTPUT
	volatile uint8_t SW0;
	volatile uint8_t SW1;
	volatile uint8_t SW2;
	volatile uint8_t SW3;
	volatile uint8_t SW4;
	volatile uint8_t SW5;
	volatile uint8_t SW6;
	volatile uint8_t SW7;
}INPUT; /*end struct*/

ISR(PCINT1_vect){/*INTERRUPT SERVICE ROUTINE*/
	ATOMIC_BLOCK(ATOMIC_FORCEON){/*WE DO NOT WANT TO BE INTERRUPTED*/
		/*INPUT.SW0 = (PIND & 0b00000001)
		INPUT.SW1 = (PIND & 0b00000010);
		INPUT.SW2 = (PIND & 0b00000100);
		INPUT.SW3 = (PIND & 0b00001000);
		INPUT.SW4 = (PIND & 0b00010000);*/
		INPUT.SW3 = (3<<(PIND & 0b00000001));//PORT D0
		INPUT.SW4 = (3<<(PIND & 0b00000010));//PORT D1
		INPUT.SW5 = (3<<(PIND & 0b00000100));//PORT D2
		INPUT.SW6 = (3<<(PIND & 0b00001000));//PORT D3
		INPUT.SW7 = (3<<(PIND & 0b00010000));//PORT D4
	    	
		INPUT.SW2 = ((PINC & 0b00001000)>>3);//PORT C3
	    	INPUT.SW1 = ((PINC & 0b00010000)>>3);//PORT C4
	    	INPUT.SW0 = ((PINC & 0b00100000)>>3);//PORT C5
		update = true;/*TELL MAIN THAT THERE IS SOMETHING TO DO*/
	}/*end ATOMIC BLOCK*/
}/*end ISR for switch1*/
ISR(PCINT2_vect){/*INTERRUPT SERVICE ROUTINE*/
	ATOMIC_BLOCK(ATOMIC_FORCEON){/*WE DO NOT WANT TO BE INTERRUPTED*/
	/*	INPUT.SW0 = (PIND & 0b00000001);
		INPUT.SW1 = (PIND & 0b00000010);
		INPUT.SW2 = (PIND & 0b00000100);
		INPUT.SW3 = (PIND & 0b00001000);
		INPUT.SW4 = (PIND & 0b00010000);*/
		//SWITCH NUMBER CORRESPONDS TO BIT POSITION!
		INPUT.SW3 = (3<<(PIND & 0b00000001));//PORT D0
		INPUT.SW4 = (3<<(PIND & 0b00000010));//PORT D1
		INPUT.SW5 = (3<<(PIND & 0b00000100));//PORT D2
		INPUT.SW6 = (3<<(PIND & 0b00001000));//PORT D3
		INPUT.SW7 = (3<<(PIND & 0b00010000));//PORT D4
	    	
		INPUT.SW2 = ((PINC & 0b00001000)>>3);//PORT C3
	    	INPUT.SW1 = ((PINC & 0b00010000)>>3);//PORT C4
	    	INPUT.SW0 = ((PINC & 0b00100000)>>3);//PORT C5
		update = true;/*TELL MAIN THAT THERE IS SOMETHING TO DO*/
	}/*end ATOMIC BLOCK*/
}/*end ISR for switch1*/
/*MAIN LOOP, INITIALIZE IO, ENABLE INTERRUPTS, SPIN CLOCK CYCLES*/
int main (void)
{
	/*THIS WORKED BEFORE FOR THE SWITCH-> LED CODE 
	DDRB = 0XFF;
	OUTPUT_PORT = 0x00; 
	
	DDRD = 0x00;  
	INPUT_PORT = 0xFF; 

	EICRA |= (1<<ISC00); 
	EIMSK |= (1<<INT0);  
	PCICR |= (1<<PCIE2);
	PCMSK2 |= (1<<PCINT16 | 1<<PCINT17 | 1<<PCINT19 | 1<<PCINT20); 
	*/
/******************************************************************************
 * 	ADDED FROM AVR_PWM4.C TO TEST IO:
 * ***************************************************************************/	
	volatile uint8_t TEMP;
	volatile uint8_t SUSTAIN;
	volatile uint8_t INVERT;
	/*PORT DEFS on PAGE 92 of ATMEL DATASHEET*/	
	//7  6  5  4  3  2  1  0 | '0' IS INPUT, '1' IS OUTPUT:
	//0  0  0  0  1  1  1  0
	DDRB  |= ((1<<PB3) | (1<<PB2) | (1<<PB1));  /*PORT B[3:1] ARE OUTPUTS*/
	PORTB = 0x0; /*INITIALIZE ALL TO "OFF" STATE*/
	
	//PC[5:3] ARE INPUTS SO MAKE SURE 0'S ARE WRITTEN:
	DDRC &= 0b10000111;
	PORTC = 0b00111000;//TURN ON PULLUPS

	//PD0[4:1] ARE INPUTS, PD[6:5] ARE OUTPUTS:
	//7  6  5  4  3  2  1  0
	//0  1  1  0  0  0  0  0
	DDRD = 0b01100000;
	PORTD = 0b00011111; /*0x1E-TURN ON PULLUPS ON INPUTS, OFF STATE FOR OUTPUTS*/
	
	PCICR |= ((1<<PCIE1) | (1<<PCIE2));/*ENABLE PCINTS ON BANK 1,2*/
	
	PCMSK1 |= ((1<<PCINT11) | (1<<PCINT12) | (1<<PCINT13)); /*BANK 1, bits 3,4,5*/
	PCMSK2 |= ((1<<PCINT16) | (1<<PCINT17) | (1<<PCINT18) | (1<<PCINT19) | (1<<PCINT20)); 
	sei(); 		     /*ENABLE GLOBAL INTERRUPTS*/	
	
	while(1){ 	
		if(update){
			ATOMIC_BLOCK(ATOMIC_FORCEON){
			//LOGIC VALUE IS LOW IF PUSHED, SO INVERT, STORE IN TEMP:
			TEMP=0X0;//CLEAR TEMP VARIABLE!
			TEMP  = ~(INPUT.SW7 | INPUT.SW6 | INPUT.SW5 | INPUT.SW4 | INPUT.SW3 | INPUT.SW2 | INPUT.SW2 | INPUT.SW1);
			//ALIGN D6,5 WITH SW7,6, WHICH ARE D4,3
			PORTD = ((TEMP & 0b11000000)>>1);//
			//ALIGN B3,2,1 WITH SW5,4,3 WHICH ARE D2,1,0
			PORTB = ((TEMP & 0b00111000)>>2);
			//SUSTAIN = (TEMP & 0b00000100);
			//INVERT =  (TEMP & 0b00000010);	
			update = false;
			}/*end atomic block*/
		}/*end if*/
	}/*end while*/
}/*end main*/
/**************************************************************************
* 	EOF
**************************************************************************/
