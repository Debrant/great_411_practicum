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
}INPUT; /*end struct*/

void Move_interrupts(void)
{	/*FROM P.70 OF ATMEL DATASHEET*/
	MCUCR = (1<<IVCE); /*ENABLE CHANGE OF INT VECTORS*/
	MCUCR = (1<<IVSEL); /*MOVE INTS TO BOOT FLASH SECTION*/
}/*end move interrupts*/

ISR(INT0_vect){/*INTERRUPT SERVICE ROUTINE*/
	ATOMIC_BLOCK(ATOMIC_FORCEON){/*WE DO NOT WANT TO BE INTERRUPTED*/
		INPUT.prior_state = temp_out;
		INPUT.SW0 = (PIND & 0b00000001); /*BIT 1*/
		INPUT.SW1 = (PIND & 0b00000010); /*BIT 2*/
		INPUT.SW2 = (PIND & 0b00000100); /*BIT 3*/
		INPUT.SW3 = (PIND & 0b00001000); /*BIT 4*/
		INPUT.SW4 = (PIND & 0b00010000);/*BIT 5*/
//		temp_out = INPUT_PORT;
		update = true;/*TELL MAIN THAT THERE IS SOMETHING TO DO*/
	}/*end ATOMIC BLOCK*/
}/*end ISR for switch1*/
ISR(PCINT2_vect){/*INTERRUPT SERVICE ROUTINE*/
	ATOMIC_BLOCK(ATOMIC_FORCEON){/*WE DO NOT WANT TO BE INTERRUPTED*/
		INPUT.prior_state = temp_out;
		INPUT.SW0 = (PIND & 0b00000001); /*BIT 1*/
		INPUT.SW1 = (PIND & 0b00000010); /*BIT 2*/
		INPUT.SW2 = (PIND & 0b00000100); /*BIT 3*/
		INPUT.SW3 = (PIND & 0b00001000); /*BIT 4*/
		INPUT.SW4 = (PIND & 0b00010000);/*BIT 5*/
		//temp_out = INPUT_PORT;
		update = true;/*TELL MAIN THAT THERE IS SOMETHING TO DO*/
	}/*end ATOMIC BLOCK*/
}/*end ISR for switch1*/

/*MAIN LOOP, INITIALIZE IO, ENABLE INTERRUPTS, SPIN CLOCK CYCLES*/
int main (void)
{
 	/*PORT DEFS on PAGE 92 of ATMEL DATASHEET*/	
	DDRB = 0XFF;  /*PORT B IS NOW ALL OUTPUTS*/
	OUTPUT_PORT = 0x00; /*INITIALIZE ALL TO "OFF" STATE*/
	
	DDRD = 0x00;  /*PORTD IS NOW ALL INPUTS, OUR INTERRUPTS*/ 
	INPUT_PORT = 0xFF; /*TURN ON PULLUPS*/

	EICRA |= (1<<ISC00); /*EXTERNAL INTERRUPT CONTROL REGISTER A*/	
	EIMSK |= (1<<INT0);  /* TURN ON INTERRUPT 0*/
	PCICR |= (1<<PCIE2);
	PCMSK2 |= (1<<PCINT16 | 1<<PCINT17 | 1<<PCINT19 | 1<<PCINT20); 

	sei(); 		     /*ENABLE GLOBAL INTERRUPTS*/	
	
	while(1){ 	
		if(update){
			ATOMIC_BLOCK(ATOMIC_FORCEON){
				OUTPUT_PORT = ~(INPUT.SW0 | INPUT.SW1 | INPUT.SW2 | INPUT.SW3 | INPUT.SW4);
				update = false;
			}/*end atomic block*/
		}/*end if*/
	}/*end while*/
}/*end main*/
/**************************************************************************
* 	EOF
**************************************************************************/
