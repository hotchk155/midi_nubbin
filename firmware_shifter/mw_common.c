#include <system.h>
#include <memory.h>
#include "mw_common.h"

// define the buffer used to receive MIDI input
#define SZ_RXBUFFER 			0x20	// size of MIDI receive buffer (power of 2)
#define SZ_RXBUFFER_MASK 		0x1F	// mask to keep an index within range of buffer
volatile byte rx_buffer[SZ_RXBUFFER];	// the MIDI receive buffer
volatile byte rx_head = 0;				// buffer data insertion index
volatile byte rx_tail = 0;				// buffer data retrieval index


#define SZ_TXBUFFER 			0x20	// size of MIDI transmit buffer (power of 2)
#define SZ_TXBUFFER_MASK 		0x1F	// mask to keep an index within range of buffer
volatile byte tx_buffer[SZ_RXBUFFER];	// the MIDI receive buffer
volatile byte tx_head = 0;				// buffer data insertion index
volatile byte tx_tail = 0;				// buffer data retrieval index


// State flags used while receiving MIDI data
byte midi_status = 0;					// current MIDI message status (running status)
byte midi_num_params = 0;				// number of parameters needed by current MIDI message
byte midi_params[2];					// parameter values of current MIDI message
char midi_param = 0;					// number of params currently received
byte is_sysex = 0;


byte timer_ticked = 0;
int led_timeout = 0;
int debounce=0;
byte last_key=0;
#define LED_FLASH 20
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
		timer_ticked = 1;
		intcon.2 = 0;
		
		
	}

	// SERIAL PORT TRANSMIT COMPLETE
	if(pir1.5)
	{
		if(tx_head != tx_tail) {
			txreg = tx_buffer[tx_tail];
			tx_tail = (tx_tail+1)&SZ_TXBUFFER_MASK;
		}
		pir1.5 = 0;
	}
	
	// SERIAL PORT RECEIVE
	if(pir1.5)
	{
		byte b = rcreg;				
		byte i = (rx_head+1)&SZ_RXBUFFER_MASK;
		if(i!=rx_tail) {
			rx_buffer[rx_head] = b;
			rx_head = i;
		}
		pir1.5 = 0;			
	}
}

////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void usart_init()
{
	pir1.1 = 0;		//TXIF 		
	pir1.5 = 0;		//RCIF
	
	pie1.1 = 1;		//TXIE 		enable
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
void usart_transmit(byte d) {
	if(tx_head == tx_tail) {
		txreg = d;
	}
	else {
		byte i = (tx_head+1)&SZ_TXBUFFER_MASK;
		if(i==tx_tail) {
			// buffer overflow!
		}
		else {
			tx_buffer[tx_tail] = d;
			tx_tail = i;
		}
	}
}

byte usart_receive(byte *d) {
	// usart buffer overrun error?
	if(rcsta.1)
	{
		rcsta.4 = 0;
		rcsta.4 = 1;
	}
	// check for empty receive buffer
	if(rx_head == rx_tail)
		return 0;
	// read the character out of buffer
	*d = rx_buffer[rx_tail];
	rx_tail=(rx_tail+1)&SZ_RXBUFFER_MASK;
	return 1;
}

////////////////////////////////////////////////////////////
// GET MESSAGES FROM MIDI INPUT
void midi_thru()
{
	// loop until there is no more data or
	// we receive a full message
	for(;;)
	{
		byte ch;
		byte ignore_ch = 0;
		if(!usart_receive(&ch)) {
			return;
		}

		// SYSTEM MESSAGE
		if((ch & 0xf0) == 0xf0)
		{
			switch(ch)
			{
			// RELEVANT REALTIME MESSAGE 
			case MIDI_SYNCH_TICK:
			case MIDI_SYNCH_START:
			case MIDI_SYNCH_CONTINUE:
			case MIDI_SYNCH_STOP:
				if(!on_clock(ch)) {
					ignore_ch = 1;
				}
				break;	
			// SYSTEM COMMON MESSAGES WITH PARAMETERS
			case MIDI_MTC_QTR_FRAME:	// 1 param byte follows
			case MIDI_SONG_SELECT:		// 1 param byte follows			
			case MIDI_SPP:				// 2 param bytes follow
				midi_param = 0;
				midi_status = ch; 
				midi_num_params = (ch==MIDI_SPP)? 2:1;
				break;
			// START OF SYSEX	
			case MIDI_SYSEX_BEGIN:
				is_sysex = 1;
				break;
			// END OF SYSEX	
			case MIDI_SYSEX_END:
				is_sysex = 0;
				break;
			}
			// Ignoring....			
			//  0xF4	RESERVED
			//  0xF5	RESERVED
			//  0xF6	TUNE REQUEST
			//	0xF9	RESERVED
			//	0xFD	RESERVED
			//	0xFE	ACTIVE SENSING
			//	0xFF	RESET
		}    
		// STATUS BYTE
		else if(!!(ch & 0x80))
		{
			// a status byte cancels sysex state
			is_sysex = 0;
		
			midi_param = 0;
			midi_status = ch; 
			switch(ch & 0xF0)
			{
			case 0xC0: //  Patch change  1  instrument #   
			case 0xD0: //  Channel Pressure  1  pressure  
				midi_num_params = 1;
				break;    
			case 0xA0: //  Polyphonic aftertouch  2  key  touch  
			case 0x80: //  Note-off  2  key  velocity  
			case 0x90: //  Note-on  2  key  veolcity  
			case 0xB0: //  Continuous controller  2  controller #  controller value  
			case 0xE0: //  Pitch bend  2  lsb (7 bits)  msb (7 bits)  
			default:
				midi_num_params = 2;
				break;        
			}
		}
		else if(midi_status)
		{
			// custom filtering of MIDI messages
			switch(midi_status & 0xF0) {
				case 0x80: //  Note-off  2  key  velocity  
				case 0x90: //  Note-on  2  key  veolcity  
					if(midi_param == 0) {
						on_note(midi_status, &ch);
					}
					break;
			}

			midi_params[midi_param] = ch;
			if(++midi_param >= midi_num_params) {
				midi_param = 0;
			}
		}
		
		// send byte through to output
		if(!ignore_ch) {
			usart_transmit(ch);
		}
		
	}
}


void led_flash() {
	P_LED = 1;
	led_timeout = LED_FLASH;
}

void mw_init() {
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
	usart_init();

	// Flash MIDI activity LED on startup
	P_LED=1; P_LED=1; delay_ms(255);
	P_LED=0; P_LED=0; delay_ms(255);
	P_LED=1; P_LED=1; delay_ms(255);
	P_LED=0; P_LED=0; 

	int debounce = 0;
	// loop forever		
	
	int last_key = 0;
	#define DEBOUNCE_MS 20
	
	int led_timeout = 0;

}

////////////////////////////////////////////////////////////
// ENTRY POINT
void mw_run()
{ 
/*
TODO - ensure that keys for a command are released before 
allowing a further command; stop sometimes registered with
quick presses of start
*/	
	
	if(timer_ticked) {
		timer_ticked = 0;
		
		if(debounce) {
			--debounce;
		}
		
		if(led_timeout) {
			if(!--led_timeout) {
				P_LED = 0;
			}
		}		
	}
	
	if(!debounce) 
	{
		byte this_key = (~porta) & M_KEYS;
		if(this_key != last_key) {
			debounce = DEBOUNCE_MS;						
			last_key = this_key;
			on_key(this_key);
		}
	}
	
}


