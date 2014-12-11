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

#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include<stdint.h>
#include<stdbool.h>
#include<util/atomic.h>

#define C_NOTE 30
#define D_NOTE 27
#define E_NOTE 24
#define G_NOTE 20
#define A_NOTE 18

/*USE VOLATILE SO COMPILER DOES NOT OPTIMIZE OUT:*/
volatile bool update = false;
volatile bool ADC_arb = false;

volatile uint16_t ADC_0;
volatile uint16_t ADC_1;
volatile uint16_t MOD1; 	//ADC INPUT BUFFER VARIABLE
volatile int MOD2 = 2; 		//CLOCK SCALER MODIFIER VARIABLE
volatile uint16_t MOD3;

/*ALTERNATE FUNCTION BUTTONS:*/
volatile uint8_t SW0;
volatile uint8_t SW1;
volatile uint8_t SW2;

/*THESE ARE OUR FINGER BUTTONS:*/
volatile uint8_t SW3;
volatile uint8_t SW4;
volatile uint8_t SW5;
volatile uint8_t SW6;
volatile uint8_t SW7;

/******************************************************************************
 * 	THIS STRUCT HOUSES OUR VALUE FOR CURRENT OUTPUT SETTINGS:
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
 * 	INITIALIZE PWM SETTINGS:
 * ***************************************************************************/
void pwm_init(void){
	TCCR0A |= ((1<<COM0A0) | (1<<COM0A1) | (1<<WGM00) | (1<<WGM01));/*PD*/
	TCNT0 = 0;	
	TCCR2A |= ((1<<COM2A1) | (1<<WGM20) | (1<<WGM21));//WORKS!
	TCNT2 = 0;	
	OUTPUT.PULSE_WIDTH0 = (F_CPU/C_NOTE); 
	OUTPUT.PULSE_WIDTH1 = (F_CPU/D_NOTE);
	OUTPUT.PULSE_WIDTH2 = (F_CPU/E_NOTE);
	OUTPUT.PULSE_WIDTH3 = (F_CPU/G_NOTE);
	OUTPUT.PULSE_WIDTH4 = (F_CPU/A_NOTE);
}/*end init*/
/******************************************************************************
 * 	INTIALIZE ADC PARAMETERS FOR INPUT:
 * ***************************************************************************/
void adc_init(){
	ADMUX |= (1<<ADLAR); //SET REGISTER TO LEFT ADJUST, USE AS 8-BIT, !16
	
	ADMUX &= 0xF0; 	     //MASK OFF LOWER BITS FOR CONFIGURATION
	ADMUX |= (1<<REFS0); //SET REFERENCE TO AVCC W/ CAP TO GROUND
	ADMUX |= (1<<MUX0);  //INITIALIZE TO ADC1, PC1, PIN 24
	ADCSRA |= (1<<ADEN); //P255,256
	_delay_ms(1); 	     //ADEN ENABLE TAKES ~12 ADC CLK CYCLES, @~200KHZ
	ADCSRA |= (1<<ADATE);//SET TO AUTO TRIGGER ADC CONVERSION EVENT
	ADCSRB &= 0xF8;      //FREE RUNNING MODE
	ADCSRA |= (1<<ADPS0);//1MHZ / 200KHZ = 5, SO ROUND UP TO 8 FOR PRESCLR
	ADCSRA |= (1<<ADPS1);//OTHER PORTION OF PRESCALER
	ADCSRA |= (1<<ADIE); //ADC INTERRUPT ENABLE
}/*end adc_start*/
/******************************************************************************
 * 	KILL ADC INPUT ENABLE AND AUTO TRIGGER:
 * ***************************************************************************/
void adc_kill(){
	ADCSRA &= ~((1<<ADEN) | (1<<ADATE));
}//END ADC_KILL()
/******************************************************************************
 * 	UPDATE PWM MODIFIERS AFTER SWITCH PRESS:
 * ***************************************************************************/
void pwm_update(){
	if(SW0){
		 ++MOD2;
		if(MOD2>2) MOD2=0;	
	}
	if(SW1){
		 (--MOD2);
		 if(MOD2<0) MOD2=2;
	}	
	if(SW2){ 
		MOD1=ADC_0;
		MOD3=ADC_1;
	}
	//else{
	//	MOD1=1;
	//	MOD3=1;
	//}
	if(SW3) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH0;		
	if(SW4) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH1;
	if(SW5) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH2;
	if(SW6) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH3;
	if(SW7) OUTPUT.PULSE = OUTPUT.PULSE_WIDTH4;
}//END PWM_UPDATE
/******************************************************************************
 * 	IMPLEMENT PWM MODIFIERS TO OUTPUT WAVEFORM:
 * ***************************************************************************/
void pwm_output(){
	switch(MOD2){
		case 0: 
			//OCR0A = (MOD3*(OUTPUT.PULSE*MOD1))%255;
			OCR0A = (10*MOD1*OUTPUT.PULSE);
			TCCR0B ^= _BV(CS01);
		break;
		case 1:
			//OCR0A = (MOD3*(OUTPUT.PULSE/MOD1))%128;
			OCR0A = (MOD1*OUTPUT.PULSE);
			TCCR0B ^= _BV(CS01);
		break;
		case 2:
			//OCR0A = ((10*MOD1)*(OUTPUT.PULSE/MOD3)%64);
			OCR0A = ((OUTPUT.PULSE/MOD1));
			TCCR0B ^= _BV(CS01);
		break;
		default: break;
	}//END SWITCH
}/*END PWM_OUTPUT*/
/******************************************************************************
 * 	INTERRUPT VECTORS:
 * ***************************************************************************/
ISR(PCINT1_vect){
	//UBER CRUDE DEBOUNCING: CHECK, DELAY, CHECK AGAIN..
	//IT IS TERRIBLE PRACTICE TO DELAY IN AN ISR BUT... YA...
	cli();
	SW0 = ((~PINC) & 0b00001000);//PORT C3
	_delay_ms(2);
	SW0 = ((~PINC) & 0b00001000);//PORT C3
	SW1 = ((~PINC) & 0b00010000);//PORT C5
	_delay_ms(2);
	SW1 = ((~PINC) & 0b00010000);//PORT C5
	SW2 = ((~PINC) & 0b00100000);//PORT C5
	_delay_ms(2);
	SW2 = ((~PINC) & 0b00100000);//PORT C5
	
	SW3 = ((~PIND) & 0b00000001);//PORT D0
	_delay_ms(2);
	SW3 = ((~PIND) & 0b00000001);//PORT D0
	SW4 = ((~PIND) & 0b00000010);//PORT D1
	_delay_ms(2);
	SW4 = ((~PIND) & 0b00000010);//PORT D1
	SW5 = ((~PIND) & 0b00000100);//PORT D2
	_delay_ms(2);
	SW6 = ((~PIND) & 0b00001000);//PORT D3
	SW6 = ((~PIND) & 0b00001000);//PORT D3
	_delay_ms(2);
	SW7 = ((~PIND) & 0b00010000);//PORT D4    	
	_delay_ms(2);
	SW7 = ((~PIND) & 0b00010000);//PORT D4    	
	update = true;
	sei();
}//end pcint1_vect

ISR(PCINT2_vect){
	cli();
	//UBER CRUDE DEBOUNCING: CHECK, DELAY, CHECK AGAIN..
	SW0 = ((~PINC) & 0b00001000);//PORT C3
	_delay_ms(2);
	SW0 = ((~PINC) & 0b00001000);//PORT C3
	SW1 = ((~PINC) & 0b00010000);//PORT C5
	_delay_ms(2);
	SW1 = ((~PINC) & 0b00010000);//PORT C5
	SW2 = ((~PINC) & 0b00100000);//PORT C5
	_delay_ms(2);
	SW2 = ((~PINC) & 0b00100000);//PORT C5
	
	SW3 = ((~PIND) & 0b00000001);//PORT D0
	_delay_ms(2);
	SW3 = ((~PIND) & 0b00000001);//PORT D0
	SW4 = ((~PIND) & 0b00000010);//PORT D1
	_delay_ms(2);
	SW4 = ((~PIND) & 0b00000010);//PORT D1
	SW5 = ((~PIND) & 0b00000100);//PORT D2
	_delay_ms(2);
	SW5 = ((~PIND) & 0b00000100);//PORT D2
	SW6 = ((~PIND) & 0b00001000);//PORT D3
	_delay_ms(2);
	SW6 = ((~PIND) & 0b00001000);//PORT D3
	SW7 = ((~PIND) & 0b00010000);//PORT D4    	
	_delay_ms(2);
	SW7 = ((~PIND) & 0b00010000);//PORT D4    	
	update = true;		
	sei();
}//end pcint2_vect

/******************************************************************************
 * 	ADC INTERRUPT VECTOR, TRIGGERS WHEN CONVERSION COMPLETE:
 * ***************************************************************************/
ISR(ADC_vect){
	cli();
	//if(!ADC_arb){
	//	ADMUX &= ~(1<<MUX0);
	//	ADMUX |= (1<<MUX1); 
		ADC_0 = ADCH;//PRIOR CONVERSION COMPLETES BEFORE CHANGE, SO, OK
	//}
	//else{
	//	ADMUX &= ~(1<<MUX1);
	//	ADMUX |= (1<<MUX0); 
	//	ADC_1 = ADCH;
	//}
	//ADC_arb = (!ADC_arb);//INITIALIZED AS FALSE, SO SHOULD START WITH ADC_0
	sei();
}//ADC_vect()
/******************************************************************************
 * 	MAIN LOOP, PRIMARILY IO PORT INITIALIZATIONS AND WHILE(1) LOOP:
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
	adc_init();/*INITIALIZE ADC INPUT REGISTERS*/
	
	sei(); 	   /*ENABLE GLOBAL INTERRUPTS*/	
	ADCSRA |= ((1<<ADSC));/*START CONVERSION*/
	
	while(1){ 	
		//REENABLE IF USING ADC_KILL() P255,256
		if(update){
			ATOMIC_BLOCK(ATOMIC_FORCEON){
				pwm_update();
				pwm_output();
				update = false;
			}//END ATOMIC BLOCK
		}//END IF
	}/*end while*/
}/*end main*/
/******************************************************************************
* 	EOF
******************************************************************************/
