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
 * avr-gcc -mmcu=atmega328 -Wall -Os -std=gnu99 -g3 -gdwarf-2 -o AVR_INTS2.elf AVR_INTS2.c
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
 *@5 NOVEMBER 2014:
 * REBUILT LAST WEEKS LOST CHANGES...
 * NOW HAVE 2 PWM OUTPUTS WITH AUDIBLE TONES
 * NOW HAVE PINS ON PROPER PORTS FOR IO, (DIFFERENT THAN LED CODE!)
 * TODO: SWITCH ALL PINS TO NEW ONES FROM HARDWARE


 * **********************************************************************/

//#define F_CPU 2000000UL
#define F_CPU 1000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include<stdint.h>
#include<stdbool.h>
#include<util/atomic.h>

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
#define G4 392
#define A4 440
#define B4 494
#define C5 523
#define D5 587

/*USE VOLATILE SO COMPILER DOES NOT OPTIMIZE OUT*/
volatile bool update = false;
volatile uint8_t temp_out;
volatile uint32_t TOP;
volatile uint8_t ADC_IN;
volatile uint8_t ADC_RDY;

volatile struct input {
	 uint8_t PULSE_WIDTH;
	 uint8_t SW0;
	 uint8_t SW1;
	 uint8_t SW2;
	 uint8_t SW3;
	 uint8_t SW4;
	 uint8_t SW5;
	 uint8_t SW6;
	 uint8_t SW7;
}INPUT; /*end struct*/

/*THESE ARE OUR FINGER BUTTONS: (5)*/
volatile struct tone0{
	uint32_t PULSE_WIDTH;
	volatile bool pushed;

} TONE0;
volatile struct tone1{
	uint32_t PULSE_WIDTH;
	volatile bool pushed;

} TONE1;
volatile struct tone2{
	uint32_t PULSE_WIDTH;
	volatile bool pushed;

} TONE2;
volatile struct tone3{
	uint32_t PULSE_WIDTH;
	volatile bool pushed;

} TONE3;
volatile struct tone4{
	uint32_t PULSE_WIDTH;
	volatile bool pushed;

} TONE4;
/*THESE ARE OUR 3 WRIST BUTTONS:*/
volatile struct tone5{
	uint32_t PULSE_WIDTH;
	volatile bool pushed;

} TONE5;
volatile struct tone6{
	uint32_t PULSE_WIDTH;
	volatile bool pushed;

} TONE6;
volatile struct tone7{
	uint32_t PULSE_WIDTH;
	volatile bool pushed;

} TONE7;

void pwm_init(void){
	TONE0.pushed=false;
	TONE1.pushed=false;
	TONE2.pushed=false;
	TONE3.pushed=false;
	TONE4.pushed=false;
	TONE5.pushed=false;
	TONE6.pushed=false;
	TONE7.pushed=false;
	
	TONE0.PULSE_WIDTH = (F_CPU/(16*G4));//159
	TONE1.PULSE_WIDTH = (F_CPU/(16*A4));//142
	TONE2.PULSE_WIDTH = (F_CPU/(16*B4));//126
	TONE3.PULSE_WIDTH = (F_CPU/(16*C5));//119
	TONE4.PULSE_WIDTH = (F_CPU/(16*D5));//106
	TONE5.PULSE_WIDTH = (F_CPU/(32*D5));//53
	TONE6.PULSE_WIDTH = (F_CPU/(32*C5));//59
	TONE7.PULSE_WIDTH = (F_CPU/(32*B4));//63


	/*PRTIMx BIT MUST BE WRITTEN 0 TO ENABLE TIMER/COUNTERS P.94 DS*/
	/*OUTPUT COMPARE REGISTERS, OUR (5) PWM OUTPUTS:*/
	OCR0A = TONE0.PULSE_WIDTH;//PIN 10, PD6
	OCR0B = TONE1.PULSE_WIDTH;//PIN 9,  PD5
	OCR1A = TONE2.PULSE_WIDTH;//PIN 13, PB1
	OCR1B = TONE3.PULSE_WIDTH;//PIN 14, PB2
	OCR2A = TONE4.PULSE_WIDTH;//PIN 15, PB3

	/*TIMER INTERRUPT FLAG REGISTERS:*/
	/*P.110***********COMPARE0B**COMPARE0A*TIMER OVERFLOW*******************/
	/*BITS:[7:3]RES, [2]OCF0B, [1]OCF0A, [0]TOV0*/
	/*MAY NEED AN ISR THAT IS A CATCH-ALL, DIRECTS BASED ON FLAGS...*/
	/*SET WHEN OVERFLOW OCCURS, CLEARED BY INTERRUPT TIFRnx:*/
	//TIFR0 |= ();//8-BIT->PORTS:  [OC0A,PD6], [OC0B,PD5]
	//TIFR1 |= ();//16-BIT->PORTS: [OC1A,PB1], [OC1B,PB2]
	//TIFR2 |= ();//8-BIT->PORTS:  [OC2A,PB3], [OC2B,PD3]

	TIMSK0 |= ((1<<OCIE0A) | (1<<OCIE0B));
	TIMSK1 |= ((1<<OCIE1A) | (1<<OCIE1B));
	TIMSK2 |= ((1<<OCIE2A) | (1<<OCIE2B));
	
	/*TIMER COUNTER CLOCK SOURCES: 8-BIT, INVERTED, PHASE CORRECT, FAST PWM*/
	TCCR0A |= ((1<<COM0A0) | (1<<COM0B0)| (1<<WGM00) | (1<<WGM01));/*PD*/
	//TCCR0A |= ((1<<COM0A1) | (1<<COM0A0) | (1<<COM0B1) | (1<<COM0B0)| (1<<WGM00) | (1<<WGM01));/*PD*/
	TCCR0B |= ((1<<WGM02)); 		/*SELECT: FORCE CLOCK OFF*/
	TCNT0 = 0;	

	//TODO: verify 16-bit settings!
	//CS11 IN TCCR1B WILL SELECT PRESCALER OF 8, PRIOR NO CLOCK IS SELECTED, SO OFF	
	/*TIMER COUNTER CLOCK SOURCES: 8-BIT, INVERTED, PHASE CORRECT, FAST PWM*/
	TCCR1A |= ((1<<COM1A0) | (1<<COM1B0)| (1<<WGM10) | (1<<WGM11) | (1<<WGM13));
	//TCCR1A |= ((1<<COM1A1) | (1<<COM1A0) | (1<<COM1B1) | (1<<COM1B0)| (1<<WGM10) | (1<<WGM11) | (1<<WGM13));
	TCCR1B |= ((1<<WGM12)); 		
	TCNT1H = 0;
	TCNT1L = 0;	/*USE THE LOW REGISTER, ACT AS IF ONLY 8-BIT, NOT 16*/
	
	/*TIMER COUNTER CLOCK SOURCES: 8-BIT, INVERTED, PHASE CORRECT, FAST PWM*/
	TCCR2A |= ((1<<COM2A1) | (1<<COM2A0) | (1<<COM2B1) | (1<<COM2B0) | (1<<WGM20) | (1<<WGM21));
	//TCCR2A |= ((1<<COM2A0) | (1<<COM2B0) | (1<<WGM20) | (1<<WGM21));
	TCCR2B |= ((1<<WGM22));
	TCNT2 = 0;	
}/*end init*/

void adc_init(){
/*
	//ADMUX |= ((1<<MUX0) | (1<<REFS0)); //ADC1 ON PIN PC1, PIN 24, (TQFP AND DIP)
	ADMUX = 0b01100001; //ADC1 ON PIN PC1, PIN 24, (TQFP AND DIP), P.254
	ADCSRA |= ((1<<ADEN) | (1<<ADIE) | (1<<ADATE | (1<<ADPS0)));//P255,256
	//ADCSRB |= ((1<<ADTS2) | (1<<ADTS1));
	ADCSRB = 0x00; //FREE RUNNING MODE
	DIDR0 |= ((1<<ADC1D) | (1<<ADC0D));
	//SREG |= (1<<I); //SHOULD NOT NEED, BUT JUST IN CASE
*/
}/*end adc_start*/

void adc_kill(){
	ADCSRA &= ~((1<<ADEN) | (1<<ADATE));
}

void pwm_update(void){
	ATOMIC_BLOCK(ATOMIC_FORCEON){
		/*TODO: TRY SETTING ONE OCRx SETTING TO '0', SPOOF MULTI IO*/
		if(1==INPUT.SW0){
			//OCR0A = ((TONE0.PULSE_WIDTH*ADC_IN)%255);
			TCCR0B |= ((1<<CS01) | (1<<CS00)); 
		}
		else if(1==INPUT.SW1){
			TCCR1B |= ((1<<CS11) | (1<<CS10)); 
		}
		else if(1==INPUT.SW2){
			TCCR1B |= ((1<<CS11) | (1<<CS10));
		}
		else if(1==INPUT.SW3){
			TCCR2B |= ((1<<CS21) | (1<<CS20));
		}
		else if(1==INPUT.SW4){
			TCCR2B |= ((1<<CS21) | (1<<CS20)); 
		}
		else{ 
			TCCR0B |= ((1<<CS01) | (1<<CS00));
			TCCR1B |= ((1<<CS11) | (1<<CS10));
			TCCR2B |= ((1<<CS21) | (1<<CS20));
	//TODO ONCE DEBUGGED, PUT THESE BACK> (DFLT TO OFF)
	//		TCCR2B &= ~((1<<CS20) | (1<<CS21));
	//		TCCR1B &= ~((1<<CS10) | (1<<CS11)); 
	//		TCCR0B &= ~((1<<CS00) | (1<<CS01));
		}/*END ELSE*/
	}/*END ATOMIC BLOCK*/
}/*END PWM_UPDATE*/
void pwm_kill(){
	/*KILL ALL PWM TIMER CLOCKS:*/	
	TCCR0B &= ~((1<<CS00) | (1<<CS01) | (1<<CS02)); 
	TCCR1B &= ~((1<<CS10) | (1<<CS11) | (1<<CS12)); 
	TCCR2B &= ~((1<<CS20) | (1<<CS21) | (1<<CS22));
}/*END PWM_KILL*/

ISR(PCINT1_vect){
	ATOMIC_BLOCK(ATOMIC_FORCEON){
		//PINS D0,1,2,3,4 AND PC3,4,5 ARE OUR INPUTS: 
		INPUT.SW3 = (3<<(PIND & 0b00000001));//PORT D0
		INPUT.SW4 = (3<<(PIND & 0b00000010));//PORT D1
		INPUT.SW5 = (3<<(PIND & 0b00000100));//PORT D2
		INPUT.SW6 = (3<<(PIND & 0b00001000));//PORT D3
		INPUT.SW7 = (3<<(PIND & 0b00010000));//PORT D4
	    	
		INPUT.SW2 = ((PINC & 0b00001000)>>3);//PORT C3
	    	INPUT.SW1 = ((PINC & 0b00010000)>>3);//PORT C4
	    	INPUT.SW0 = ((PINC & 0b00100000)>>3);//PORT C5
	update = true;	
	}//END ATOMIC
}//end pcint11_vect
ISR(PCINT2_vect){
	ATOMIC_BLOCK(ATOMIC_FORCEON){
		INPUT.SW3 = (3<<(PIND & 0b00000001));//PORT D0
		INPUT.SW4 = (3<<(PIND & 0b00000010));//PORT D1
		INPUT.SW5 = (3<<(PIND & 0b00000100));//PORT D2
		INPUT.SW6 = (3<<(PIND & 0b00001000));//PORT D3
		INPUT.SW7 = (3<<(PIND & 0b00010000));//PORT D4
	    	
		INPUT.SW2 = ((PINC & 0b00001000)>>3);//PORT C3
	    	INPUT.SW1 = ((PINC & 0b00010000)>>3);//PORT C4
	    	INPUT.SW0 = ((PINC & 0b00100000)>>3);//PORT C5
	update = true;		
	}//END ATOMIC
}//pcint12_vect

ISR(ADC_vect){
	cli();
	ADC_IN = ADCH;
	ADMUX ^= (1<<MUX0); 
	update = true;
	sei();
}//END ADC ISR

/*MAIN LOOP, INITIALIZE IO, ENABLE INTERRUPTS, SPIN CLOCK CYCLES*/
int main (void)
{
	cli(); /*DISABLE ALL INTERRUPTS UNTIL INITIALIZED:*/ 
	
	/*PORT DEFS on PAGE 92 of ATMEL DATASHEET*/	
	//7  6  5  4  3  2  1  0 | '0' IS INPUT, '1' IS OUTPUT:
	//0  0  0  0  1  1  1  0
	DDRB  |= ((1<<PB3) | (1<<PB2) | (1<<PB1));  /*PORT B[3:1] ARE OUTPUTS*/
	PORTB = 0x0; /*INITIALIZE ALL TO "OFF" STATE*/
	
	//PC[5:3] ARE INPUTS SO MAKE SURE 0'S ARE WRITTEN:
	DDRC = 0b00000000;//TODO MAKE SURE PC6 NOT LOW, RESET!
	PORTC = 0b00111000;//TURN ON PULLUPS

	//PD0[4:1] ARE INPUTS, PD[6:5] ARE OUTPUTS:
	//7  6  5  4  3  2  1  0
	//0  1  1  0  0  0  0  0
	DDRD = 0b01100000;
	PORTD = 0b00011111; /*0x1E-TURN ON PULLUPS ON INPUTS, OFF STATE FOR OUTPUTS*/
	
	PCICR |= ((1<<PCIE1) | (1<<PCIE2));/*ENABLE PCINTS ON BANK 1,2*/
	
	PCMSK1 |= ((1<<PCINT11) | (1<<PCINT12) | (1<<PCINT13)); /*BANK 1, bits 3,4,5*/
	PCMSK2 |= ((1<<PCINT16) | (1<<PCINT17) | (1<<PCINT18) | (1<<PCINT19) | (1<<PCINT20)); 
	
	//TIMSK1 
	//TODO: ADD IN TOIE1? TOIE0?
	
	TIMSK0 |= ((1<<OCIE0A) | (1<<OCIE0B)); 
	TIMSK1 |= ((1<<OCIE1A) | (1<<OCIE1B)); 
	TIMSK2 |= ((1<<OCIE2A) | (1<<OCIE2B)); 
	
	pwm_init();/*INITIALIZE PWM REGISTERS*/
	//adc_init();/*INITIALIZE ADC INPUT REGISTERS*/
	sei(); 	   /*ENABLE GLOBAL INTERRUPTS*/	
	
	while(1){ 	
//		if(update){
			ATOMIC_BLOCK(ATOMIC_FORCEON){
			//pwm_update();
//			OCR1 = 0XFFFF;
			OCR1A = 0X3FFF;
			OCR1B = 0XBFFF;
			TCCR1A |= (1<<WGM11);
			TCCR1B |= ((1<<WGM12) | (1<<WGM13));
			TCCR1B |= (1<<CS10);			
			//_delay_ms(100);//WANT TO GET RID OF THIS, BUT MAY NEED
//			pwm_kill();
			update = false;
//			}
		}/*end if*/
	}/*end while*/
}/*end main*/
/******************************************************************************
* 	EOF
******************************************************************************/
