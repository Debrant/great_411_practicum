/*************************************************************************
 *  	J.S.PEIRCE
 * 	AVR SWITCH INPUT TEST PROGRAM
 * 	ECE411 - FALL 2014
 * 	TEAM T01
 * 	26 OCTOBER 2014
 *
 * 	TOOLCHAIN:
 * 		AVR-GCC 
 * 		AVR-GDB
 * 		AVR-LIBC
 * 	BOARD:
 * 		AVRDRAGON BOARD via USB (ISP CONFIGURATION)
 * 
 * 1) COMPILE  WITH:
 * avr-gcc -mmcu=atmega328 -Wall -Os -o AVR_INTS1.elf AVR_INTS1.c
 * 
 * 2) LINK WITH:
 * avr-objcopy -j .text -j .data -O ihex AVR_INTS1.elf AVR_INTS1.hex
 *
 * 3) CONNECT AVRDRAGON...
 *
 * -MUST RUN BURN SCRIPT (burn) AS SUDO OR ROOT!:
 * -add directory to PATH or from directory containing ALL files type:
 * 
 * 4) sudo ./burn
 * -you will be prompted for passwd if not root, enter it...
 * -script will prompt for part, use: m328
 * -script will prompt for .HEX file, use: AVR_POLL.hex
 * -script will confirm args, then run if correct
 *
 ************************************************************************
 ***************CHANGELOG:***********************************************
 ************************************************************************
 *27 OCTOBER 2014:
 * ADDED INPUT SUPPORT 
 ***********************************************************************/
#define F_CPU 1000000UL

//PCINT1_vect SIG_PIN_CHANGE1 //pin change interrupt
//INT1_vect SIG_INTERRUPT1 //EXT INT REQ 1 (vect 2, ADDR 0x0002) P.65
//ADC_vect SIG_ADC //ADC CONVERSION COMPLETE

#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
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


*************************************************************************/
void Move_interrupts(void)
{	/*FROM P.70 OF ATMEL DATASHEET*/
	MCUCR = (1<<IVCE); /*ENABLE CHANGE OF INT VECTORS*/
	MCUCR = (1<<IVSEL); /*MOVE INTS TO BOOT FLASH SECTION*/
}/*end move interrupts*/

ISR(INT0_vect) 
{
	OUTPUT_PORT ^= _BV(PB0);
	//TODO: have it change prescaler once tested!
}/*end ISR for switch1*/


int main (void)
{
 	/*PORT DEFS on PAGE 92 of ATMEL DATASHEET*/	
	DDRB = 0XFF;  /*PORT B IS NOW ALL OUTPUTS*/
	PORTB = 0x00; /*INITIALIZE ALL TO "OFF" STATE*/
	
	DDRD &= ~(1 << DDD2);  /*PCINT0 IS NOW AN INPUT*/ 
	PORTD |= (1<< PORTD2); /*TURN ON PULLUP*/

	EICRA |= (1<<ISC00); /*EXTERNAL INTERRUPT CONTROL REGISTER A*/	
	EIMSK |= (1<<INT0);  /* TURN ON INT0*/

	sei(); 		   /*ENABLE GLOBAL INTERRUPTS*/	
	
	while(1)
	{ 	
		_delay_ms(50);	
	}/*end while*/
}/*end main*/

/**************************************************************************
* 	EOF
**************************************************************************/
