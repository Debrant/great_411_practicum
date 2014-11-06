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
 *@5 NOVEMBER 2014:
 * REBUILT LAST WEEKS LOST CHANGES...
 * NOW HAVE 2 PWM OUTPUTS WITH AUDIBLE TONES
 * NOW HAVE PINS ON PROPER PORTS FOR IO, (DIFFERENT THAN LED CODE!)
 * TODO: ADC IN ON TIMER1 (16-BIT) FOR POT
 * TODO: DEBOUNCE SWITCHES
 * TODO: GET REMAINING PWM OUT WORKING (2-3)
 * TODO: FIX MAKEFILE FOR PROPER FORMAT
 * TODO: BEAT THIS UGLY THING INTO SUBMISSION...
 * **********************************************************************/

//#define F_CPU 2000000UL
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
#define G4 392
#define A4 440
#define B4 494
#define C5 523
#define D5 587

/*USE VOLATILE SO COMPILER DOES NOT OPTIMIZE OUT*/
volatile bool update = false;
volatile uint8_t temp_out;
volatile uint32_t TOP;

volatile struct input {
	 uint8_t PULSE_WIDTH;
	 uint8_t SW0;
	 uint8_t SW1;
	 uint8_t SW2;
	 uint8_t SW3;
	 uint8_t SW4;
}INPUT; /*end struct*/

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
void Move_interrupts(void)
{	/*FROM P.70 OF ATMEL DATASHEET*/
	MCUCR = (1<<IVCE); /*ENABLE CHANGE OF INT VECTORS*/
	MCUCR = (1<<IVSEL); /*MOVE INTS TO BOOT FLASH SECTION*/
}/*end move interrupts*/
void pwm_init(void){
	TONE0.pushed=false;
	TONE1.pushed=false;
	TONE2.pushed=false;
	TONE3.pushed=false;
	TONE4.pushed=false;
	
	TONE0.PULSE_WIDTH = (F_CPU/(16*G4));
	TONE1.PULSE_WIDTH = (F_CPU/(16*A4));
	TONE2.PULSE_WIDTH = (F_CPU/(16*B4));
	TONE3.PULSE_WIDTH = (F_CPU/(16*C5));
	TONE4.PULSE_WIDTH = (F_CPU/(16*D5));

	/*GTCCR - GENERAL TIMER-COUNTER CONTROL REGISTER:*/
	/*BITS: [7]TSM, [6:2]RES, [1]RES, [0]PSRSYNC*/

	/*PRTIMx BIT MUST BE WRITTEN 0 TO ENABLE TIMER/COUNTERS P.94 DS*/
	/*TIMER COUNTER REGISTERS->ACTIVE WHEN: (TCNTn == OCRnx)*/
	//TCNT0 |= (); /*CORRESPONDS TO: [OCR0A,B]*/
	//TCNT1 |= (); /*CORRESPONDS TO: [OCR1A,B]*/
	//TCNT2 |= (); /*CORRESPONDS TO: [OCR2A,B]*/
	
	/*OUTPUT COMPARE REGISTERS, CTC MODE: SET TO TOGGLE OUTPUT AT "VALUE":*/
	OCR0A = TONE0.PULSE_WIDTH;
	OCR0B = TONE1.PULSE_WIDTH;
	//OCR1A
	//OCR1B
	OCR2A = TONE2.PULSE_WIDTH;
	OCR2B = TONE3.PULSE_WIDTH;

	/*OUTPUT COMPARE FLAG SET ON MATCH, TRIGERS INTERRUPT:*/
	//OCF0A
	//OCF0B
	//OCF1A
	//OCF1B
	//OCF2A
	//OCF2B
	
	/*TIMER INTERRUPT FLAG REGISTERS:*/
	/*P.110***********COMPARE0B**COMPARE0A*TIMER OVERFLOW*******************/
	/*BITS:[7:3]RES, [2]OCF0B, [1]OCF0A, [0]TOV0*/
	/*MAY NEED AN ISR THAT IS A CATCH-ALL, DIRECTS BASED ON FLAGS...*/
	/*SET WHEN OVERFLOW OCCURS, CLEARED BY INTERRUPT TIFRnx:*/
	//TOVn, BIT[0]
	//TIFR0 |= ();//8-BIT->PORTS:  [OC0A,PD6], [OC0B,PD5]
	//TIFR1 |= ();//16-BIT->PORTS: [OC1A,PB1], [OC1B,PB2]
	//TIFR2 |= ();//8-BIT->PORTS:  [OC2A,PB3], [OC2B,PD3]
	
	/*TIMER COUNTER CLOCK SOURCES:*/
	//CURRENTLY NON-PWM, OVERFLOW AT TOP (OCRx), CTC MODE->TOGGLE OUTPUT
	TCCR0A |= ((1<<COM0A0) | (1<<COM0A1) | (1<<COM0B0) | (1<<COM0B1)| (1<<WGM00) | (1<<WGM01));/*PD*/
	TCCR0B |= ((1<<CS01) | (1<<WGM02)); 		/*SELECT: FORCE CLOCK OFF*/
	//TCCR0B |= ((1<<CS01) | (1<<CS00) | (1<<WGM02)); 		/*SELECT: FORCE CLOCK OFF*/
	TCNT0 = 0;	
	//TO TURN ON:
	//TCCR0B |= (1<<CS00 | 1<<CS01);/*SELECT: INTERNAL CLK/64*/
	
	/*16-BIT REGISTERS, USE FOR ADC?...*/
	//TCCR1B |= (1<<COM0A0 | 1<<COM0B0 1<<WGM01);/*SET: WGM01 AND 00*/
	//TCCR1B |= (1<<CS00 | 1<<CS01);/*SELECT: INTERNAL CLK/64*/
	/*8-BIT REGISTERS, SET2:CTC, TOGGLE, */
	TCCR2A |= ((1<<COM2A0) | (1<<COM2A1) | (1<<COM2B1) | (1<<COM2B0) | (1<<WGM20) | (1<<WGM21));
	TCCR2B |= ((1<<CS21) | (1<<WGM22));
	//TCCR2B |= ((1<<CS21) | (1<<CS20) | 1<<WGM22);
	TCNT2 = 0;	
	//TO TURN ON w/ CLK/64:
	//TCCR2B |= (1<<CS22);
}/*end init*/

void adc_start(uint8_t PULSE_WIDTH){
	/*NOTE: HIGH MUST BE WRITTEN BEFORE LOW! (ALWAYS ON 16-BIT REG)*/
	//OCR1AH = 0;
	//OCR1AL = 0;
	//TCCR1A = 0x81;  /*8-BIT, NON-INVERTED PWM*/
	//TCCR1B = 1; /*START PWM*/
}/*end pwm_start*/

void pwm_update(void){
	ATOMIC_BLOCK(ATOMIC_FORCEON){
		if(INPUT.SW0){
			OCR0A = TONE0.PULSE_WIDTH;
			TCCR0B |= ((1<<CS01) | (1<<CS00)); 
		}
		else if(INPUT.SW1){
			OCR0B = TONE1.PULSE_WIDTH;	
			TCCR0B |= ((1<<CS01) | (1<<CS00)); 
		}
		//else TCCR0B &= ~(1<<CS00); 
		else if(INPUT.SW2){
			OCR2A = TONE2.PULSE_WIDTH;	
			TCCR2B |= ((1<<CS21) | (1<<CS20));
		}
		else if(INPUT.SW3){
			OCR2B = TONE3.PULSE_WIDTH;
			TCCR2B |= ((1<<CS21) | (1<<CS20));
		}
		else{ (TCCR2B &= ~(1<<CS20));
			 (TCCR0B &= ~(1<<CS00)); 
		}
	}
}/*END PWM_UPDATE*/

ISR(INT0_vect){/*INTERRUPT SERVICE ROUTINE*/
	ATOMIC_BLOCK(ATOMIC_FORCEON){/*WE DO NOT WANT TO BE INTERRUPTED*/
	INPUT.SW0 = (~PIND & 0b00000001);/*BIT 1*/
	INPUT.SW1 = (~PIND & 0b00000010);/*BIT 2*/
	INPUT.SW2 = (~PIND & 0b00000100);/*BIT 3*/
	INPUT.SW3 = (~PIND & 0b00010000);/*BIT 4*/
//	TONE4.pushed = (INPUT.SW4 = (PINB & 0b00010000)? true:false);/*BIT 5*/
		update = true;/*TELL MAIN THAT THERE IS SOMETHING TO DO*/
	}/*end ATOMIC BLOCK*/
}/*end ISR for switch1*/
ISR(PCINT2_vect){/*INTERRUPT SERVICE ROUTINE*/
	ATOMIC_BLOCK(ATOMIC_FORCEON){/*WE DO NOT WANT TO BE INTERRUPTED*/
	INPUT.SW0 = (~PIND & 0b00000001);/*BIT 1*/
	INPUT.SW1 = (~PIND & 0b00000010);/*BIT 2*/
	INPUT.SW2 = (~PIND & 0b00000100);/*BIT 3*/
	INPUT.SW3 = (~PIND & 0b00010000);/*BIT 4*/
//	TONE4.pushed = (INPUT.SW4 = (PINB & 0b00010000)? true:false);/*BIT 5*/
		update = true;/*TELL MAIN THAT THERE IS SOMETHING TO DO*/
	}/*end ATOMIC BLOCK*/
}/*end ISR for switch1*/
ISR(PCINT0_vect){/*INTERRUPT SERVICE ROUTINE*/
	ATOMIC_BLOCK(ATOMIC_FORCEON){/*WE DO NOT WANT TO BE INTERRUPTED*/
	INPUT.SW0 = (~PIND & 0b00000001);/*BIT 1*/
	INPUT.SW1 = (~PIND & 0b00000010);/*BIT 2*/
	INPUT.SW2 = (~PIND & 0b00000100);/*BIT 3*/
	INPUT.SW3 = (~PIND & 0b00010000);/*BIT 4*/
//	TONE4.pushed = (INPUT.SW4 = (PINB & 0b00010000)? true:false);/*BIT 5*/
		update = true;/*TELL MAIN THAT THERE IS SOMETHING TO DO*/
	}/*end ATOMIC BLOCK*/
}/*end ISR for switch1*/
ISR(TIMER0_COMPB_vect){
}

ISR(TIMER2_OVF_vect){
}

ISR(ADC_vect){

reti();
}

/*MAIN LOOP, INITIALIZE IO, ENABLE INTERRUPTS, SPIN CLOCK CYCLES*/
int main (void)
{
	cli(); /*DISABLE ALL INTERRUPTS UNTIL INITIALIZED:*/ 
	
	pwm_init();
	/*PORT DEFS on PAGE 92 of ATMEL DATASHEET*/	
	//7  6  5  4  3  2  1  0
	//0  0  0  1  0  0  0  0
	DDRB  &= ~(1<<PB4);  /*PORT B IS NOW ALL OUTPUTS, EXCEPT PB4!*/
	PORTB = 0b00010000; /*INITIALIZE ALL TO "OFF" STATE, PB4 PULLUP ON*/
	
	//PD3, 5, 6 ARE INPUTS, REST OUTPUTS:
	//7  6  5  4  3  2  1  0
	//0  1  1  0  1  0  0  0
	DDRD = 0b01101000;
	//DDRD  |=  ((1<<PB7) | (1<<PB5) | (1<<PB3));  /*PORTD3,5,6 ARE OUTPUTS, OTHERS ARE INPUT INTERRUPTS*/ 
//	PORTD = 0b01101111;
	PORTD = 0b10010111; /*TURN ON PULLUPS ON INPUTS, OFF STATE FOR OUTPUTS*/

	EICRA |= (1<<ISC00); /*EXTERNAL INTERRUPT CONTROL REGISTER A*/	
	EIMSK |= (1<<INT0);  /* TURN ON INTERRUPT 0, PIN PD2*/
	PCICR |= (1<<PCIE2 | 1<<PCIE0);/*ENABLE PCINTS ON BANK 0,2*/
	
	PCMSK0 |= (1<<PCINT4); /*BANK 0, PCINT 4 IS PIN PB4*/
	PCMSK2 |= ((1<<PCINT16) | (1<<PCINT17) | (1<<PCINT20)); 
	TIMSK0 |= ((1<<OCIE0A) | (1<<OCIE0B));
	TIMSK2 |= ((1<<OCIE2A) | (1<<OCIE2B) | (1<<TOIE2)); 
	
	sei(); 		     /*ENABLE GLOBAL INTERRUPTS*/	
	
	while(1){ 	
		if(update){
			ATOMIC_BLOCK(ATOMIC_FORCEON){
			pwm_update();
			//_delay_ms(10);
			update = false;
			}
		}/*end if*/
	}/*end while*/
}/*end main*/
/******************************************************************************
* 	EOF
******************************************************************************/
