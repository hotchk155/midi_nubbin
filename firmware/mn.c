#include <xc.h>
#include <string.h>
#include "mn.h"
/*
 
       PIC16F15345
 
        VDD | VSS
LED1    RA5 | RA0
KEY1    RA4 | RA1
        RA3 | RA2
RX      RC5 | RC0       LED2
TX      RC4 | RC1
KEY2    RC3 | RC2 
KEY3    RC6 | RB4
KEY4    RC7 | RB5   
KEY5    RB7 | RB6
 
 */
// COMPILER SETTINGS
// Memory Model
//      - ROM Ranges: default,-1F80-1FFF

#define SAF_ADDR 0x1f80 // address of Storage Area Flash block

#define P_LED1 LATAbits.LATA5
#define P_LED2 LATCbits.LATC0
#define P_KEY1 PORTCbits.RC3
#define P_KEY2 PORTAbits.RA4
#define P_KEY3 PORTCbits.RC6
#define P_KEY4 PORTCbits.RC7
#define P_KEY5 PORTBbits.RB7

#define TRISA_BITS  0b11011111
#define TRISB_BITS  0b11111111
#define TRISC_BITS  0b11101110

#define WPUA_BITS   0b00010000
#define WPUB_BITS   0b10000000
#define WPUC_BITS   0b11001000

#define DEBOUNCE_TIMEOUT 50
#define LONG_PRESS_TIMEOUT 1000
#define LED_BLINK_MS 100


enum {
    RX_Q_DEPTH =    0x20,
    RX_Q_MASK =     0x1f,
    TX_Q_DEPTH =    0x20,
    TX_Q_MASK =     0x1f    
};

volatile byte g_rx_queue[RX_Q_DEPTH];
volatile int g_rx_head;
volatile int g_rx_tail;
volatile byte g_tx_queue[TX_Q_DEPTH];
volatile int g_tx_head;
volatile int g_tx_tail;
volatile byte g_ms_tick;
byte g_last_keys;
byte g_led1_timeout;
byte g_led2_timeout;
int g_debounce_timeout;
int g_long_press_timeout;
byte g_midi_status = 0;					// current MIDI message status (running status)
byte g_midi_num_params = 0;				// number of parameters needed by current MIDI message
byte g_midi_params[2];					// parameter values of current MIDI message
byte g_midi_param = 0;					// number of params currently received
byte g_in_sysex = 0;

//////////////////////////////////////////
void rx_push(byte data) {
    int next = (g_rx_head+1)&RX_Q_MASK;
    if(next != g_rx_tail) {
        g_rx_queue[g_rx_head] = data;
        g_rx_head = next;
    }
}
//////////////////////////////////////////
byte rx_pop() {
    byte data = 0;
    if(g_rx_tail != g_rx_head) {
        data = g_rx_queue[g_rx_tail];
        g_rx_tail = (g_rx_tail+1)&RX_Q_MASK;
    }
    return data;
}
//////////////////////////////////////////
byte rx_avail() {
    return (g_rx_tail != g_rx_head);
}
//////////////////////////////////////////
void tx_push(byte data) {
    int next = (g_tx_head+1)&TX_Q_MASK;
    if(next != g_tx_tail) {
        g_tx_queue[g_tx_head] = data;
        g_tx_head = next;
    }
}
//////////////////////////////////////////
byte tx_pop() {
    byte data = 0;
    if(g_tx_tail != g_tx_head) {
        data = g_tx_queue[g_tx_tail];
        g_tx_tail = (g_tx_tail+1)&TX_Q_MASK;
    }
    return data;
}
//////////////////////////////////////////
byte tx_avail() {
    return (g_tx_tail != g_tx_head);
}
//////////////////////////////////////////
void __interrupt() ISR()
{
    // when a character is available at the serial port
    if(PIR3bits.RC1IF) {
        rx_push(RC1REG);
        PIR3bits.RC1IF = 0;
    }
    // when a character finishes sending
    if(PIR3bits.TX1IF) {
        if(tx_avail()) {
            TX1REG = tx_pop();
        }
        else {
            // disable interrupt for now
            PIE3bits.TX1IE = 0;
        }
        PIR3bits.TX1IF = 0;
    }
	// once per millisecond
	if(PIR0bits.TMR0IF)
	{
		PIR0bits.TMR0IF = 0;
		g_ms_tick = 1;
	}		

}

//////////////////////////////////////////
void uart_init() {
     
    TX1STAbits.TX9 = 0;     // 8 bit transmission
    TX1STAbits.TXEN = 1;    // transmit enable    
    TX1STAbits.SYNC = 0;    // async mode
    TX1STAbits.SENDB = 0;   // break character
    TX1STAbits.BRGH = 0;    // low baudrate 
    
    RC1STAbits.RX9 = 0;     // 8 bit operation
    RC1STAbits.CREN = 1;    // continuous receive enable

    BAUD1CONbits.SCKP = 0;  // bit polarity 
    BAUD1CONbits.BRG16 = 1; // enable 16 bit brg
    BAUD1CONbits.WUE = 0;   // wake up enable off
    BAUD1CONbits.ABDEN = 0; // auto baud detect

    SP1BRGH = 0;            // brg high byte
    SP1BRGL = 63;            // brg low byte (31250)	 

    RC1STAbits.SPEN = 1;    // serial port enable    
    
    PIR3bits.RC1IF = 0;     
    PIE3bits.RC1IE = 1;     

    PIR3bits.TX1IF = 0;     
    PIE3bits.TX1IE = 0;     // only enabled when sending
}
////////////////////////////////////////////////////////////
void timer_init() {
	// Configure timer 0 (controls systemticks)
    //  main clock 32MHz
	// 	timer 0 runs at 8MHz (Fosc/4)
	// 	prescaled 1/32 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	    
    T0CON0bits.T016BIT = 0;  // 8-bit mode
    //T0CON1bits.T0CS = 0b010;    // } Fosc/4 clock source
    T0CON1bits.T0CS0 = 0;    // } Fosc/4 clock source
    T0CON1bits.T0CS1 = 1;    // }
    T0CON1bits.T0CS2 = 0;    // }
    //T0CON1bits.T0CKPS = 0b0101;
    T0CON1bits.T0CKPS0 = 1;    // } Prescaler 1/32
    T0CON1bits.T0CKPS1 = 0;    // }
    T0CON1bits.T0CKPS2 = 1;    // }
    T0CON1bits.T0CKPS3 = 0;    // }
        
    TMR0H = 250;
    TMR0L = 0;
    T0CON0bits.T0EN = 1;    
    
    PIR0bits.TMR0IF = 0;
    PIE0bits.TMR0IE = 1;
}

/////////////////////////////////////////////////////
void service_leds() {
    if(g_led1_timeout) {
        if(!--g_led1_timeout) {
            P_LED1 = 0;
        }
    }
    if(g_led2_timeout) {
        if(!--g_led2_timeout) {
            P_LED2 = 0;
        }
    }
}
/////////////////////////////////////////////////////
void service_buttons() {
    if(!g_debounce_timeout) {
        byte this_keys = 0;
        if(!P_KEY1) {
            this_keys |= KEY_1;
        }
        if(!P_KEY2) {
            this_keys |= KEY_2;
        }
        if(!P_KEY3) {
            this_keys |= KEY_3;
        }
        if(!P_KEY4) {
            this_keys |= KEY_4;
        }
        if(!P_KEY5) {
            this_keys |= KEY_5;
        }
        
        byte diff_keys = g_last_keys ^ this_keys;
        if(diff_keys) {
            g_debounce_timeout = DEBOUNCE_TIMEOUT;
            g_long_press_timeout = LONG_PRESS_TIMEOUT;
            byte keys_down = diff_keys & this_keys;
            if(keys_down) {
                app_key_event(EV_KEY_DOWN, keys_down);
            }
            g_last_keys = this_keys;
        }
        else if(this_keys) {            
            if(g_long_press_timeout) {
                if(!--g_long_press_timeout) {
                    app_key_event(EV_KEY_HOLD, this_keys);                    
                }
            }
        }
    }
    else {
        --g_debounce_timeout;
    }
}

////////////////////////////////////////////////////////////
void send_char(byte data) {
    //di();
    tx_push(data);
    //ei();
    PIE3bits.TX1IE = 1;
}    

///////////////////////////////////////////////////
// GET MESSAGES FROM MIDI INPUT
void service_midi_in()
{
	// loop until there is no more data or
	// we receive a full message
	for(;;)
	{
		// usart buffer overrun error?
        if(RC1STAbits.OERR) {
            RC1STAbits.CREN = 0;
            RC1STAbits.CREN = 1;
        }

        // check if character available and pull it from the 
        // buffer, disable interrupts for safe access to buffer
        //di();
        byte ch;
        byte is_avail = rx_avail();
        if(is_avail) {
            ch = rx_pop();
        }
        //ei();
        
        // exit from loop if no data available
        if(!is_avail) {
            break;
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
                // these messages can appear at any point in the data
                app_midi_realtime(ch);
                break;
			// SYSTEM COMMON MESSAGES WITH PARAMETERS
			case MIDI_MTC_QTR_FRAME:	// 1 param byte follows
			case MIDI_SONG_SELECT:		// 1 param byte follows			
				g_midi_param = 0;
				g_midi_status = ch; 
				g_midi_num_params = 1;
				break;
			case MIDI_SPP:				// 2 param bytes follow
				g_midi_param = 0;
				g_midi_status = ch; 
				g_midi_num_params = 2;
				break;
			// START OF SYSEX	
			case MIDI_SYSEX_BEGIN:
				g_in_sysex = 1;
				break;
			// END OF SYSEX	
			case MIDI_SYSEX_END:
                g_in_sysex = 0;
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
			g_in_sysex = 0;
			g_midi_param = 0;
			g_midi_status = ch; 
			switch(ch & 0xF0)
			{
			case 0xC0: //  Patch change  1  instrument #   
			case 0xD0: //  Channel Pressure  1  pressure  
				g_midi_num_params = 1;
				break;    
			case 0xA0: //  Polyphonic aftertouch  2  key  touch  
			case 0x80: //  Note-off  2  key  velocity  
			case 0x90: //  Note-on  2  key  veolcity  
			case 0xB0: //  Continuous controller  2  controller #  controller value  
			case 0xE0: //  Pitch bend  2  lsb (7 bits)  msb (7 bits)  
			default:
				g_midi_num_params = 2;
				break;        
			}
		}    
        else if(g_in_sysex) {
            send_char(ch);
        }
		else if(g_midi_status)
        {
            // gathering parameters
            g_midi_params[g_midi_param++] = ch;
            if(g_midi_param >= g_midi_num_params)
            {
                // we have a complete message
                g_midi_param = 0;
                app_midi_msg(g_midi_status, g_midi_num_params, g_midi_params[0], g_midi_params[1]);
            }
        }
	}
}


////////////////////////////////////////////////////////
// provide access to the first 32 word page of the SAF
void mn_saf_read(byte *addr, size_t size) 
{	
    byte buf[64];
    
    NVMCON1bits.NVMREGS = 0;        // access program flash memory
	NVMADRL = (byte)SAF_ADDR;           // set word address 
	NVMADRH = (byte)(SAF_ADDR >> 8);	
	
	// Loop through all 32 words -> 64 bytes
	for(int i=0; i<64; i+=2) 
	{	        
        NVMCON1bits.RD = 1;             // initiate the read
		buf[i] = NVMDATH;       // move the word into memory
		buf[i+1] = NVMDATL;
        ++NVMADRL;                      // advance to the next word address
	}
    memcpy(addr, buf, size);
}

////////////////////////////////////////////////////////
// ERASE AND WRITE A ROW OF FLASH MEMORY
// The target address is in the addr variable and must
// be on a flash row boundary (32 words)
// The data is read from 64 byte array buffer.data
void mn_saf_write(byte *addr, size_t size)
{
    byte buf[64] = {0};
    if(size>64) {
        size = 64;
    }
    memcpy(buf, addr, size);
    
    di();
    
    // prepare the row erase
    NVMADRL = (byte)SAF_ADDR; // set up base address of the row to erase (32-word boundary)
    NVMADRH = (byte)(SAF_ADDR >> 8);
    NVMCON1bits.NVMREGS = 0;        // access program flash memory
    NVMCON1bits.FREE = 1;           // flag this is an erase operation
    NVMCON1bits.WREN = 1;           // enable write
	NVMCON2 = 0x55;                 // special unlock sequence
	NVMCON2 = 0xAA;		    
	NVMCON1bits.WR = 1;                 // kick off the erase

    // LOAD THE WRITE LATCHES
    // Prepare to write new data into the cleared row. The sequence is to 
    // load the 32 word write buffer (latches), then kick off the write	
	NVMCON1bits.LWLO = 1;           // indicate we're setting up the latches
	for(int i=0; i<64; i+=2) 
	{
		NVMDATL = buf[i+1]; // set up the word to load into the next word latch
		NVMDATH = buf[i];
		NVMCON2 = 0x55;             // special unlock sequence
		NVMCON2 = 0xAA;		
		NVMCON1bits.WR = 1;         // perform the write to the latches. execution halts till complete
		++NVMADRL;                  // and on to the next address...
	}

	// PERFORM THE WRITE TO FLASH
	NVMCON1bits.LWLO = 0;           // ready to write from latches to FLASH
    NVMADRL = (byte)SAF_ADDR;        // set the row address
    NVMADRH = (byte)(SAF_ADDR >> 8);
	NVMDATL = buf[1];
	NVMDATH = buf[0];
	NVMCON2 = 0x55;                 // special unlock sequence
	NVMCON2 = 0xAA;		
	NVMCON1bits.WR = 1;             // start write
	
	NVMCON1bits.WREN = 0;	// disable writes
    
    ei();
}

///////////////////////////////////////////////////
void mn_send_midi_msg(byte cmd, byte num_params, byte param1, byte param2 ) {
    send_char(cmd);
    if(num_params) send_char(param1);
    if(num_params>1) send_char(param2);
}
///////////////////////////////////////////////////
void mn_send_midi_realtime(byte msg) {
    send_char(msg);
}
///////////////////////////////////////////////////
void mn_blink_left() {
    P_LED1 = 1;
    g_led1_timeout = LED_BLINK_MS;
}
///////////////////////////////////////////////////
void mn_blink_right() {
    P_LED2 = 1;
    g_led2_timeout = LED_BLINK_MS;
}
///////////////////////////////////////////////////
void mn_set_left(byte status) {
    P_LED1 = !!status;
    g_led1_timeout = 0;
}
///////////////////////////////////////////////////
void mn_set_right(byte status) {
    P_LED2 = !!status;
    g_led2_timeout = 0;
}
///////////////////////////////////////////////////
void mn_init() {
    g_rx_head = 0;
    g_rx_tail = 0;
    g_tx_head = 0;
    g_tx_tail = 0;
    g_ms_tick = 0;
    g_last_keys = 0;
    g_led1_timeout = 0;
    g_led2_timeout = 0;
    g_debounce_timeout = 0;
    g_midi_status = 0;					// current MIDI message status (running status)
    g_midi_num_params = 0;				// number of parameters needed by current MIDI message
    g_midi_param = 0;					// number of params currently received
    g_in_sysex = 0;

    TRISA=TRISA_BITS;
    TRISB=TRISB_BITS;
    TRISC=TRISC_BITS;
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    WPUA=WPUA_BITS;
    WPUB=WPUB_BITS;
    WPUC=WPUC_BITS;
    PORTA=0;  
    PORTC=0;  
    
    // Pin RC4 PPS register set to point to UART TX
    RC4PPS = 0x0F;
    
    // UART RX PPS register set to point to RC5
    RX1DTPPS = 0x15;
    

    uart_init();
    timer_init();
    app_init();

	// enable interrupts	
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    
}
///////////////////////////////////////////////////
void mn_run() {
    
    service_midi_in();        
    if(g_ms_tick) {
        service_buttons();
        service_leds();
        app_tick();
        g_ms_tick = 0;
    }
    app_run();        
}

