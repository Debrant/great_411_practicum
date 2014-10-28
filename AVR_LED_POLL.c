/***************************************************
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
 * avr-gcc -mmcu=atmega328 -Wall -Os -o AVR_POLL.elf AVR_LED_POLL.c
 * 
 * 2) LINK WITH:
 * avr-objdump -j .text -j .data -O ihex AVR_POLL.elf AVR_POLL.hex
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
 *********************************************************************
 ***************CHANGELOG:********************************************
 *********************************************************************
 *27 OCTOBER 2014:
 * ADDED INPUT SUPPORT 
 ********************************************************************/
#define F_CPU 1000000UL

#include<avr/io.h>//NEEDED? INC IN INTERRUPT.H
#include<util/delay.h>    /*NEEDED FOR DELAY FUNCTION*/
#include<util/atomic.h>   /*NEXT REVISION NEEDS*/

int main (void)
{
 	/*PORT DEFS on PAGE 92 of ATMEL DATASHEET*/
	/*VARIABLE ONLY NAMED INTERRUPT FOR FUTURE USE*/
	volatile unsigned char INTERRUPT1=0; /*CHECK EVERY TIME*/
	
	DDRB = 0xFF;  /*SET PORT B AS OUTPUT*/
	PORTB = 0x00; /*INITIALIZE TO OFF*/
	 
	DDRD = 0x00;  /*PORTD AS INPUT*/
	PORTD = 0xFF; /*ENABLE PULLUPS*/
	
	while(1){      /*SPIN AWAY THEM CLOCK CYCLES...*/ 	
		INTERRUPT1 = PIND; /*READ VALUE*/
		INTERRUPT1 &= _BV(PD3); /*WIPE OFF EXTRA BITS*/

		if((INTERRUPT1==0)){
			PORTB ^= _BV(PB0); /*XOR OUTPUT STATE*/
			_delay_ms(10); /*VERY VERY CRUDE DEBOUNCE*/
		}/*end if*/
	}/*end while*/
}/*end main*/

/*********************************************************************
* 	EOF
*********************************************************************/
