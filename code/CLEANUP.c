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
#define F_CPU 1000000UL
//#define F_CPU 8000000UL
//#define F_CPU 2000000UL
#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include<stdint.h>
#include<stdbool.h>
#include<util/atomic.h>

#define SCALE_BY_128 0x05
#define SCALE_BY_64  0x04
#define SCALE_BY_32  0x03

#define G4 392
#define A4 440
#define B4 494
#define C5 523
#define D5 587

#define G4_32 784
#define A4_32 880
#define B4_32 659 //TODO change to E...everywhere
#define C5_32 523
#define D5_32 587

#define G4_64 392
#define A4_64 440
#define B4_64 329
#define C5_64 261
#define D5_64 293

#define G4_128 196
#define A4_128 220
#define B4_128 164
#define C5_128 131
#define D5_128 147
/*USE VOLATILE SO COMPILER DOES NOT OPTIMIZE OUT*/
volatile bool update = false;

volatile uint16_t ADC_IN;
volatile uint16_t MOD1;
volatile uint16_t MOD2;

/*THESE ARE OUR FINGER BUTTONS:*/
volatile uint8_t SW0;
volatile uint8_t SW1;
volatile uint8_t SW2;
volatile uint8_t SW3;
volatile uint8_t SW4;

/*ALTERNATE FUNCTION BUTTONS:*/
volatile uint8_t SW5;
volatile uint8_t SW6;
volatile uint8_t SW7;

/******************************************************************************
 *
 * ***************************************************************************/
volatile struct output{
	uint8_t scale;
	uint32_t PULSE;
	uint32_t PULSE_WIDTH0;
	uint32_t PULSE_WIDTH1;
	uint32_t PULSE_WIDTH2;
	uint32_t PULSE_WIDTH3;
	uint32_t PULSE_WIDTH4;
}OUTPUT;
/******************************************************************************
 *
 * ***************************************************************************/
void scale(uint32_t scalar){
	if(128==scalar){
		OUTPUT.PULSE_WIDTH0 = (F_CPU/(G4_128));
		OUTPUT.PULSE_WIDTH1 = (F_CPU/(A4_128));
		OUTPUT.PULSE_WIDTH2 = (F_CPU/(B4_128));
		OUTPUT.PULSE_WIDTH3 = (F_CPU/(C5_128));
		OUTPUT.PULSE_WIDTH4 = (F_CPU/(D5_128));
	}
	if(64==scalar){
		OUTPUT.PULSE_WIDTH0 = (F_CPU/(G4_64));
		OUTPUT.PULSE_WIDTH1 = (F_CPU/(A4_64));
		OUTPUT.PULSE_WIDTH2 = (F_CPU/(B4_64));
		OUTPUT.PULSE_WIDTH3 = (F_CPU/(C5_64));
		OUTPUT.PULSE_WIDTH4 = (F_CPU/(D5_64));
	}
	if(32==scalar){
		OUTPUT.PULSE_WIDTH0 = (F_CPU/(G4_32));
		OUTPUT.PULSE_WIDTH1 = (F_CPU/(A4_32));
		OUTPUT.PULSE_WIDTH2 = (F_CPU/(B4_32));
		OUTPUT.PULSE_WIDTH3 = (F_CPU/(C5_32));
		OUTPUT.PULSE_WIDTH4 = (F_CPU/(D5_32));
	}
	else{
		OUTPUT.PULSE_WIDTH0 = (F_CPU/(G4));
		OUTPUT.PULSE_WIDTH1 = (F_CPU/(A4));
		OUTPUT.PULSE_WIDTH2 = (F_CPU/(B4));
		OUTPUT.PULSE_WIDTH3 = (F_CPU/(C5));
		OUTPUT.PULSE_WIDTH4 = (F_CPU/(D5));
	}
}
/******************************************************************************
 *
 * ***************************************************************************/
void pwm_init(void){

	TCCR0A |= ((1<<COM0A1) | (1<<WGM00) | (1<<WGM01));/*PD*/
	TCNT0 = 0;	

	TCCR1A |= ((1<<COM1A1) | (1<<WGM10) | (1<<WGM11) | (1<<WGM13));
	TCCR1B |= (1<<WGM12); 		
	TCNT1H = 0;
	TCNT1L = 0;	/*USE THE LOW REGISTER, ACT AS IF ONLY 8-BIT, NOT 16*/
	
	TCCR2A |= ((1<<COM2A1) | (1<<WGM20) | (1<<WGM01));
	TCCR2B |= ((1<<WGM22));
	TCNT2 = 0;	
	
}/*end init*/
/******************************************************************************
 *
 * ***************************************************************************/
void pwm_kill(){
	/*KILL ALL PWM TIMER CLOCKS:*/	
	if((TCCR0B & SCALE_BY_128))
		TCCR0B ^= ((1<<CS02) | (1<<CS00));
	if((TCCR0B & SCALE_BY_64))
		TCCR0B ^= (1<<CS02);
	if((TCCR0B & SCALE_BY_32))
		TCCR0B ^= ((1<<CS01) | (1<<CS00));
	TCCR0B &= 0xF8;
}/*END PWM_KILL*/
/******************************************************************************
 *
 * ***************************************************************************/
void adc_init(){
	ADMUX=0;
	ADMUX |= (1<<REFS0);
	ADMUX |= (1<<MUX0);
	ADMUX |= (1<<ADLAR);

	ADCSRA |= (1<<ADPS0);//P255,256
	ADCSRB = 0x0;//FREE RUNNING MODE
	
	ADCSRA |= ((1<<ADEN) | (1<<ADIE));//P255,256
}/*end adc_start*/
/******************************************************************************
 *
 * ***************************************************************************/
void adc_kill(){
	ADCSRA &= ~((1<<ADEN) | (1<<ADATE));
}//END ADC_KILL()
/******************************************************************************
 *
 * ***************************************************************************/
void pwm_update(){
	//TODO USE MOD OPERATOR IF POSSIBLE!
	if(SW0){
		 ++MOD2;
		if(4==MOD2) MOD2=0;
	}
	if(SW1){
		 --MOD2;//TODO ->ISSUE! MOD2 IS UNSIGNED, WOULD OVERFLOW!
		 if(-1==MOD2) MOD2=3;
	}	
	if(SW2) MOD1=ADCH;
	else MOD1=1;
}//END PWM_UPDATE
/******************************************************************************
 *
 * ***************************************************************************/
void pwm_output(){
	switch(MOD2){
		case 0:
			scale(0);
			if(SW3) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH0;		
			if(SW4) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH1;
			if(SW5) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH2;
			if(SW6) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH3;
			if(SW7) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH4;
			OCR0A = (OUTPUT.PULSE/MOD1);
			TCCR0B ^= _BV(CS01);
		break;
		case 1: 
			scale(32);
			if(SW3) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH0;		
			if(SW4) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH1;
			if(SW5) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH2;
			if(SW6) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH3;
			if(SW7) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH4;
			OCR0A = (OUTPUT.PULSE/MOD1);
			TCCR0B ^= _BV(CS01);
			TCCR0B ^= _BV(CS00);
		break;
		case 2:
			scale(64);
			if(SW3) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH0;		
			if(SW4) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH1;
			if(SW5) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH2;
			if(SW6) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH3;
			if(SW7) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH4;
			OCR0A = (OUTPUT.PULSE/MOD1);
			TCCR0B ^= _BV(CS02);
		break;
		case 3:
			scale(128);
			if(SW3) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH0;		
			if(SW4) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH1;
			if(SW5) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH2;
			if(SW6) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH3;
			if(SW7) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH4;
			OCR0A = (OUTPUT.PULSE/MOD1);
			TCCR0B ^= _BV(CS02);
			TCCR0B ^= _BV(CS00);
		break;
		default: break;
	}
}/*END PWM_OUTPUT*/
/******************************************************************************
 *
 * ***************************************************************************/

ISR(PCINT1_vect){
	//UBER CRUDE DEBOUNCING: CHECK, DELAY, CHECK AGAIN..
	//IT IS TERRIBLE PRACTICE TO DELAY IN AN ISR BUT... YA...
	cli();
	SW0 = ((~PINC) & 0b00001000);//PORT C3
	_delay_ms(1);
	SW0 = ((~PINC) & 0b00001000);//PORT C3
	SW1 = ((~PINC) & 0b00010000);//PORT C5
	_delay_ms(1);
	SW1 = ((~PINC) & 0b00010000);//PORT C5
	SW2 = ((~PINC) & 0b00100000);//PORT C5
	_delay_ms(1);
	SW2 = ((~PINC) & 0b00100000);//PORT C5
	
	SW3 = ((~PIND) & 0b00000001);//PORT D0
	_delay_ms(1);
	SW3 = ((~PIND) & 0b00000001);//PORT D0
	SW4 = ((~PIND) & 0b00000010);//PORT D1
	_delay_ms(1);
	SW4 = ((~PIND) & 0b00000010);//PORT D1
	SW5 = ((~PIND) & 0b00000100);//PORT D2
	_delay_ms(1);
	SW5 = ((~PIND) & 0b00000100);//PORT D2
	SW6 = ((~PIND) & 0b00001000);//PORT D3
	_delay_ms(1);
	SW6 = ((~PIND) & 0b00001000);//PORT D3
	SW7 = ((~PIND) & 0b00010000);//PORT D4    	
	_delay_ms(1);
	SW7 = ((~PIND) & 0b00010000);//PORT D4    	
	update = true;
	sei();
}//end pcint1_vect

ISR(PCINT2_vect){
	cli();
	//UBER CRUDE DEBOUNCING: CHECK, DELAY, CHECK AGAIN..
	SW0 = ((~PINC) & 0b00001000);//PORT C3
	_delay_ms(1);
	SW0 = ((~PINC) & 0b00001000);//PORT C3
	SW1 = ((~PINC) & 0b00010000);//PORT C5
	_delay_ms(1);
	SW1 = ((~PINC) & 0b00010000);//PORT C5
	SW2 = ((~PINC) & 0b00100000);//PORT C5
	_delay_ms(1);
	SW2 = ((~PINC) & 0b00100000);//PORT C5
	
	SW3 = ((~PIND) & 0b00000001);//PORT D0
	_delay_ms(1);
	SW3 = ((~PIND) & 0b00000001);//PORT D0
	SW4 = ((~PIND) & 0b00000010);//PORT D1
	_delay_ms(1);
	SW4 = ((~PIND) & 0b00000010);//PORT D1
	SW5 = ((~PIND) & 0b00000100);//PORT D2
	_delay_ms(1);
	SW5 = ((~PIND) & 0b00000100);//PORT D2
	SW6 = ((~PIND) & 0b00001000);//PORT D3
	_delay_ms(1);
	SW6 = ((~PIND) & 0b00001000);//PORT D3
	SW7 = ((~PIND) & 0b00010000);//PORT D4    	
	_delay_ms(1);
	SW7 = ((~PIND) & 0b00010000);//PORT D4    	
	update = true;		
	sei();
}//end pcint2_vect

/******************************************************************************
 *
 * ***************************************************************************/
ISR(ADC_vect){
	cli();
	//MOD1 = ADC_IN;
	ADC_IN = ADCH;
	sei();
}//ADC_vect()
/******************************************************************************
 *
 * ***************************************************************************/

/*MAIN LOOP, INITIALIZE IO, ENABLE INTERRUPTS, SPIN CLOCK CYCLES*/
int main (void)
{
	cli(); /*DISABLE ALL INTERRUPTS UNTIL INITIALIZED:*/ 
	/*PORT DEFS on PAGE 92 of ATMEL DATASHEET*/	
	DDRB  |= ((1<<DDB3) | (1<<DDB2) | (1<<DDB1));  /*PORT B[3:1] ARE OUTPUTS*/
	PORTB = 0x0; /*INITIALIZE ALL TO "OFF" STATE*/
	
	//PC[5:3] ARE INPUTS SO MAKE SURE 0'S ARE WRITTEN:
	//DDRC &= 0b10000000;//MAKE SURE PC6 NOT LOW, RESET!
	DDRC &= ~((1<<PC5)|(1<<PC4)|(1<<PC3));//MAKE SURE PC6 NOT LOW, RESET!
	PORTC |= ((1<<PC5) | (1<<PC4) | (1<<PC3));//TURN ON PULLUPS
	//PORTC = 0b01111000;//TURN ON PULLUPS

	//PD0[4:1] ARE INPUTS, PD[6:5] ARE OUTPUTS:
	DDRD  = 0b01100000;
	PORTD = 0b00011111; /*0x1E-TURN ON PULLUPS ON INPUTS, OFF STATE FOR OUTPUTS*/
	
	PCICR |= ((1<<PCIE1) | (1<<PCIE2));/*ENABLE PCINTS ON BANK 1,2*/
	PCMSK1 |= ((1<<PCINT11) | (1<<PCINT12) | (1<<PCINT13)); /*BANK 1, bits 3,4,5*/
	PCMSK2 |= ((1<<PCINT16) | (1<<PCINT17) | (1<<PCINT18) | (1<<PCINT19) | (1<<PCINT20)); 
	EICRA |= ((1<<ISC11));	
	
	pwm_init();/*INITIALIZE PWM REGISTERS*/
	scale(0);
	adc_init();/*INITIALIZE ADC INPUT REGISTERS*/
	
	ADCSRA |= ((1<<ADSC));/*START CONVERSION*/
	sei(); 	   /*ENABLE GLOBAL INTERRUPTS*/	
	
	while(1){ 	
		//REENABLE IF USING ADC_KILL() P255,256
		if(update){
			ATOMIC_BLOCK(ATOMIC_FORCEON){
				ADC_IN = ADCH;
				pwm_update();
				pwm_output();
				update = false;
//				pwm_kill();
			}//END ATOMIC BLOCK
		}//END IF
	}/*end while*/
}/*end main*/
/******************************************************************************
* 	EOF
******************************************************************************/
