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

// port A bits for each keys scan line
#define M_KEY0		0b00001000
#define M_KEY1		0b00010000
#define M_KEY2		0b00000100
#define M_KEYS		0b00011100

// M_KEYS bits which are pulled low for each of the 5 buttons
#define K_KEY_A 	(M_KEY1)
#define K_KEY_B 	(M_KEY0|M_KEY1)
#define K_KEY_C 	(M_KEY1|M_KEY2)
#define K_KEY_D 	(M_KEY2)
#define K_KEY_E 	(M_KEY0)
#define K_KEY_EC 	(M_KEY0|M_KEY1|M_KEY2)
#define K_KEY_ED 	(M_KEY0|M_KEY2)

#define MIDI_SYNCH_TICK     	0xf8
#define MIDI_SYNCH_START    	0xfa
#define MIDI_SYNCH_CONTINUE 	0xfb
#define MIDI_SYNCH_STOP     	0xfc

#define TICKS_PER_BEAT 24

// Timer settings
volatile byte timer_ticked = 0;		// Timer ticked flag (tick once per ms)
#define TIMER_0_INIT_SCALAR		5	// Timer 0 is an 8 bit timer counting at 250kHz

volatile int tick_adjust = 0;
volatile int is_running = 0;

#define SZ_RXBUFFER 64
#define RXBUFFER_INDEX_MASK 0x3F

#define SZ_TXBUFFER 64
#define TXBUFFER_INDEX_MASK 0x3F

volatile byte rx_buf[SZ_RXBUFFER];
volatile byte rx_head = 0;
volatile byte rx_tail = 0;

volatile byte tx_buf[SZ_TXBUFFER];
volatile byte tx_head = 0;
volatile byte tx_tail = 0;


volatile int master_ticks = 0;
volatile byte master_clock_present = 0;

////////////////////////////////////////////////////////////
void send(byte d) {

	if(tx_head == tx_tail) {
		txreg = d;
	}
	else {
		byte next_head = tx_head + 1;
		next_head &= TXBUFFER_INDEX_MASK;	
		if(next_head != tx_tail) {
			tx_buffer[tx_head] = d;
			tx_head = next_head;		
		}				
	}
}

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
	
	// SERIAL PORT RECEIVE
	if(pir1.5)
	{

		// get the byte
		byte b = rcreg;				
		switch(b) {
		case MIDI_SYNCH_TICK:
			if(!is_running) {
				// do nothing - ignore it
			}
			else if(tick_adjust<0) {
				++tick_adjust;
			}
			else {
				send(MIDI_SYNCH_TICK);
			}
			master_clock_present = 1;
			master_ticks = (master_ticks+1)%TICKS_PER_BEAT;
			break;
		case MIDI_SYNCH_START:
			is_running = 1;
			master_ticks = 0;
			break;
		case MIDI_SYNCH_STOP:
			is_running = 0;
			break;
		case MIDI_SYNCH_CONTINUE:
			is_running = 1;
			break;
		default:
			send(b);
			break;
		}
		pir1.5 = 0;			
	}
	
	////////////////////////////////////////////////
	// SERIAL PORT TRANSMIT INTERRUPT
	// TXIF bit is high whenever there is no serial
	// transmit in progress. Low->High transition will
	// trigger an interrupt, meaning characters of the 
	// transmit buffer can be sent back to back
	if(pir1.4) 
	{	
		// any pending data in transmit buffer
		if(tx_head != tx_tail) {		
			// send next character
			txreg = tx_buffer[tx_tail];
			++tx_tail;
			tx_tail &= TXBUFFER_INDEX_MASK;
		}
		pir1.4 = 0;
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
//void send(byte c)
//{
//	txreg = c;
//	while(!txsta.1);
//}

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
	
	int last_key = 0;
	#define DEBOUNCE_MS 20
	
	int led_timeout = 0;

/*
TODO - ensure that keys for a command are released before 
allowing a further command; stop sometimes registered with
quick presses of start
*/	
	for(;;)
	{
	
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
				switch(this_key) {
					case K_KEY_A:	--tick_adjust; break;
					case K_KEY_B:	send(MIDI_SYNCH_TICK); break;
					case K_KEY_C:	send(MIDI_SYNCH_START); is_running=1; break;
					case K_KEY_D:	send(MIDI_SYNCH_STOP); is_running=0; break;
					case K_KEY_E:	send(MIDI_SYNCH_CONTINUE); is_running=1; break;
				}	
			}
		}

		if(txsta.1 && tx_buf_len) {
			txreg = tx_buf[--tx_buf_len];
		}
		
		if(master_clock_present && !master_ticks) {
			P_LED = 1;
			led_timeout = is_running? 50:1;
		}
		
	}
}
