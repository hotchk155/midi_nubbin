#include <system.h>
#include <memory.h>

#define FIRMWARE_VERSION 1

// configuration words: 16MHz internal oscillator block, reset disabled
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF & _CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000
typedef unsigned char byte;

// define the I/O pins. 
#define P_LED		lata.5
#define P_KEY0		lata.3
#define P_KEY1		lata.4
#define P_KEY2		lata.2
#define P_TRISA		0b11011110
#define P_WPUA		0b00011100

#define M_KEY0		0b00001000
#define M_KEY1		0b00010000
#define M_KEY2		0b00000100
#define M_KEYS		0b00011100

// Timer settings
volatile byte timerTicked = 0;		// Timer ticked flag (tick once per ms)
#define TIMER_0_INIT_SCALAR		5	// Timer 0 is an 8 bit timer counting at 250kHz

volatile int q = 0;
////////////////////////////////////////////////////////////
// INTERRUPT HANDLER 
void interrupt( void )
{
	// TIMER0 OVERFLOW
	// Timer 0 overflow is used to 
	// create a once per millisecond
	// signal for blinking LEDs etc
	if(intcon.2)
	{
		tmr0 = TIMER_0_INIT_SCALAR;
		timerTicked = 1;
		if(q)--q;
		intcon.2 = 0;
	}
	
	// SERIAL PORT RECEIVE
	if(pir1.5)
	{

		// get the byte
		byte b = rcreg;				
		if(b == 0xF8) { 
			txreg = 0xF8;
		}
		pir1.5 = 0;			
	}
}

////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void init_usart()
{
	pir1.1 = 1;		//TXIF 		
	pir1.5 = 0;		//RCIF
	
	pie1.1 = 0;		//TXIE 		no interrupts
	pie1.5 = 1;		//RCIE 		enable
	
	baudcon.4 = 0;	// SCKP		synchronous bit polarity 
	baudcon.3 = 1;	// BRG16	enable 16 bit brg
	baudcon.1 = 0;	// WUE		wake up enable off
	baudcon.0 = 0;	// ABDEN	auto baud detect
		
	txsta.6 = 0;	// TX9		8 bit transmission
	txsta.5 = 1;	// TXEN		transmit enable
	txsta.4 = 0;	// SYNC		async mode
	txsta.3 = 0;	// SEDNB	break character
	txsta.2 = 0;	// BRGH		high baudrate 
	txsta.0 = 0;	// TX9D		bit 9

	rcsta.7 = 1;	// SPEN 	serial port enable
	rcsta.6 = 0;	// RX9 		8 bit operation
	rcsta.5 = 1;	// SREN 	enable receiver
	rcsta.4 = 1;	// CREN 	continuous receive enable
		
	spbrgh = 0;		// brg high byte
	spbrg = 31;		// brg low byte (31250)		
}

////////////////////////////////////////////////////////////
// SEND A BYTE ON SERIAL PORT
void send(byte c)
{
	txreg = c;
	while(!txsta.1);
}

////////////////////////////////////////////////////////////
// ENTRY POINT
void main()
{ 
	// osc control / 16MHz / internal
	osccon = 0b01111010;

	apfcon.7=0; // RX on RA1
	apfcon.2=0;	// TX on RA0
		
	// configure io
	trisa = 0b11011110;              	
	ansela = 0b00000000;
	
	option_reg.7 = 0;
	wpua = P_WPUA;              	
	
	// Configure timer 0 (controls systemticks)
	// 	timer 0 runs at 4MHz
	// 	prescaled 1/16 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	
	option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	option_reg.3 = 0; // timer 0 is prescaled
	option_reg.2 = 0; // }
	option_reg.1 = 1; // } 1/16 prescaler
	option_reg.0 = 1; // }
	intcon.5 = 1; 	  // enabled timer 0 interrrupt
	intcon.2 = 0;     // clear interrupt fired flag	

	// enable interrupts
	intcon.7 = 1; //global interrupt enable
	intcon.6 = 1; // peripheral interrupt enable
			
	// initialise USART
	init_usart();

	// Flash MIDI activity LED on startup
	P_LED=1; P_LED=1; delay_ms(255);
	P_LED=0; P_LED=0; delay_ms(255);
	P_LED=1; P_LED=1; delay_ms(255);
	P_LED=0; P_LED=0; 

	int debounce = 0;
	// loop forever		
	for(;;)
	{
	
		byte b = (~porta) & M_KEYS;
		switch(b) {
			case M_KEY0:					send(0xf8); break;
			case M_KEY1:					send(0xf9); break;
			case M_KEY2:					send(0xfa); break;
			case M_KEY0|M_KEY1:				send(0xfb); break;
			case M_KEY1|M_KEY2:				send(0xfc); break;
			case M_KEY0|M_KEY2:				send(0xfd); break;
			case M_KEY0|M_KEY1|M_KEY2:		send(0xfe); break;
		}	
		delay_ms(100);
	}
}