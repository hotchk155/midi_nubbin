
// PIC16F15345 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FEXTOSC = OFF    // External Oscillator mode selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINTPLL// Power-up default value for COSC bits (HFINTOSC with 2x PLL, with OSCFRQ = 16 MHz and CDIV = 1:1 (FOSC = 32 MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; i/o or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (FSCM timer enabled)

// CONFIG2
#pragma config MCLRE = OFF      // Master Clear Enable bit (MCLR pin function is port defined function)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config LPBOREN = OFF    // Low-Power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = ON       // Brown-out reset enable bits (Brown-out Reset Enabled, SBOREN bit is ignored)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (VBOR) set to 1.9V on LF, and 2.45V on F Devices)
#pragma config ZCD = OFF        // Zero-cross detect disable (Zero-cross detect circuit is disabled at POR.)
#pragma config PPS1WAY = ON     // Peripheral Pin Select one-way control (The PPSLOCK bit can be cleared and set only once in software)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a reset)

// CONFIG3
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = ON        // WDT operating mode (WDT enabled regardless of sleep; SWDTEN ignored)
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)

// CONFIG4
#pragma config BBSIZE = BB512   // Boot Block Size Selection bits (512 words boot block size)
#pragma config BBEN = OFF       // Boot Block Enable bit (Boot Block disabled)
#pragma config SAFEN = OFF      // SAF Enable bit (SAF disabled)
#pragma config WRTAPP = OFF     // Application Block Write Protection bit (Application Block not write protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block not write protected)
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration Register not write protected)
#pragma config WRTSAF = OFF     // Storage Area Flash Write Protection bit (SAF not write protected)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/Vpp must be used for programming)

// CONFIG5
#pragma config CP = OFF         // UserNVM Program memory code protection bit (UserNVM code protection disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
#define _XTAL_FREQ 32000000
#include <xc.h>

typedef uint8_t byte;


#define MIDI_SYNCH_TICK     	0xf8
#define MIDI_SYNCH_START    	0xfa
#define MIDI_SYNCH_CONTINUE 	0xfb
#define MIDI_SYNCH_STOP     	0xfc
/*

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
 
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
 
// A B
// C D E

#define P_LED1 LATAbits.LATA5
#define P_LED2 LATCbits.LATC0

enum {
    KEY_1 = (1<<0),
    KEY_2 = (1<<1),
    KEY_3 = (1<<2),
    KEY_4 = (1<<3),
    KEY_5 = (1<<4)
};

#define SZ_BUFFER 64
volatile byte tx_buf[SZ_BUFFER];
volatile int tx_head = 0;
volatile int tx_tail = 0;

void queue_push(byte ch) 
{
    int new_head = tx_head+1;
    if(new_head >= SZ_BUFFER) {
        new_head = 0;
    }
    if(new_head != tx_tail) {
        tx_buf[new_head] = ch;
        tx_head = new_head;
    }    
}

void send_byte(byte ch) {
    if(TX1STAbits.TRMT) {
        
    }
    INTCONbits.GIE = 0;
    queue_push(ch);
    INTCONbits.GIE = 1;
}

int get_next_tx() {    
    int ret = -1;
    if(tx_head != tx_tail) {
        ret = tx_buf[tx_tail];
        if(++tx_tail >= SZ_BUFFER) {
            tx_tail - 0;
        }
    }
    return ret;
}

void __interrupt() ISR()
{
    // when a character is available at the serial port
    if(PIR3bits.RC1IF) {
        volatile byte ch = RC1REG;
        switch(ch) {
            case MIDI_SYNCH_TICK:
            case MIDI_SYNCH_START:    	
            case MIDI_SYNCH_CONTINUE: 	
            case MIDI_SYNCH_STOP:     	            
                queue_byte(ch);
                break;
        }
        break;
    }
}



byte scan_keys() {
    byte result = 0;
    if(PORTAbits.RA4) {
        result |= KEY_2;
    }
    if(PORTCbits.RC3) {
        result |= KEY_1;
    }
    if(PORTCbits.RC6) {
        result |= KEY_3;
    }
    if(PORTCbits.RC7) {
        result |= KEY_4;
    }
    if(PORTBbits.RB7) {
        result |= KEY_5;
    }
    return result;
}

void uart_init() {
    PIR3bits.RC1IF = 0;     
    PIE3bits.RC1IE = 1;     

    PIR3bits.TX1IF = 0;     
    PIE3bits.TX1IE = 0;     
    
    BAUD1CONbits.SCKP = 0;  // synchronous bit polarity 
    BAUD1CONbits.BRG16 = 1; // enable 16 bit brg
    BAUD1CONbits.WUE = 0;   // wake up enable off
    BAUD1CONbits.ABDEN = 0; // auto baud detect
    
    //TX1STAbits.TX9 = 0;     // 8 bit transmission
    TX1STAbits.SYNC = 0;    // async mode
    //TX1STAbits.SENDB = 0;   // break character
    TX1STAbits.BRGH = 0;    // high baudrate 
    //TX1STAbits.TX9D = 0;    // bit 9

    
    TX1STAbits.TXEN = 1;    // transmit enable
    RC1STAbits.SPEN = 1;    // serial port enable
    
    //RC1STAbits.RX9 = 0;     // 8 bit operation
    RC1STAbits.SREN = 1;    // enable receiver
    RC1STAbits.CREN = 1;    // continuous receive enable

    SP1BRGH = 0;            // brg high byte
    SP1BRGL = 63;            // brg low byte (31250)	 
}

void send_char(byte x) {
    TX1REG = x;
    while(TX1STAbits.TRMT);
}

void main(void) {
    TRISA=0b11011111;
    TRISC=0b11101110;
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    WPUAbits.WPUA4 = 1;
    WPUCbits.WPUC3 = 1;
    WPUCbits.WPUC6 = 1;
    WPUCbits.WPUC7 = 1;
    WPUBbits.WPUB7 = 1;
    
    
    // Pin RC4 PPS register set to point to UART TX
    RC4PPS = 0x0F;
    
    // UART RX PPS register set to point to RC5
    RX1DTPPS = 0x15;
            
    uart_init();
    
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    
    //P_LED1 = 1;
    while(1) {
        if(!TX1STAbits.TRMT) {
            int next_tx = get_next_tx();
            if(next_tx >= 0) {
                TX1REG = (byte)next_tx;                
            }
        }
    }
}