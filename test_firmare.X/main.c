
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
#pragma config WDTE = OFF        // WDT operating mode (WDT enabled regardless of sleep; SWDTEN ignored)
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)

// CONFIG4
#pragma config BBSIZE = BB512   // Boot Block Size Selection bits (512 words boot block size)
#pragma config BBEN = OFF       // Boot Block Enable bit (Boot Block disabled)
#pragma config SAFEN = ON       // SAF Enable bit 
#pragma config WRTAPP = OFF     // Application Block Write Protection bit (Application Block not write protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block not write protected)
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration Register not write protected)
#pragma config WRTSAF = OFF     // Storage Area Flash Write Protection bit (SAF not write protected)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/Vpp must be used for programming)

// CONFIG5
#pragma config CP = OFF         // UserNVM Program memory code protection bit (UserNVM code protection disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
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

#define _XTAL_FREQ 32000000

//////////////////////////////////////////
void uart_init() {
     
    TX1STAbits.TX9 = 0;     // 8 bit transmission
    TX1STAbits.TXEN = 1;    // transmit enable    
    TX1STAbits.SYNC = 0;    // async mode
    TX1STAbits.SENDB = 0;   // break character
    TX1STAbits.BRGH = 0;    // low baudrate 
    
    RC1STAbits.RX9 = 0;     // 8 bit operation
    RC1STAbits.CREN = 0;    // continuous receive enable

    BAUD1CONbits.SCKP = 0;  // bit polarity 
    BAUD1CONbits.BRG16 = 1; // enable 16 bit brg
    BAUD1CONbits.WUE = 0;   // wake up enable off
    BAUD1CONbits.ABDEN = 0; // auto baud detect

    SP1BRGH = 0;            // brg high byte
    SP1BRGL = 63;            // brg low byte (31250)	 

    RC1STAbits.SPEN = 1;    // serial port enable    
    
    PIR3bits.RC1IF = 0;     
    PIE3bits.RC1IE = 0;     

    PIR3bits.TX1IF = 0;     
    PIE3bits.TX1IE = 0;     // only enabled when sending
}
void uart_send(unsigned char ch) 
{
	TXREG = ch;
	while(!TXSTAbits.TRMT);
}

///////////////////////////////////////////////////
void main() {

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
    int sent = 0;
    while(1) {
        P_LED1 = 1;
        P_LED2 = 0;
        __delay_ms(200);
        P_LED1 = 0;
        P_LED2 = 1;
        __delay_ms(200);
        while(!P_KEY1 || !P_KEY2 || !P_KEY3 || !P_KEY4 || !P_KEY5 ) {
            P_LED1 = 1;
            P_LED2 = 1;                      
            if(!sent) {
                if(!P_KEY1) sent=36;
                else if(!P_KEY2) sent=38;
                else if(!P_KEY3) sent=40;
                else if(!P_KEY4) sent=41;
                else if(!P_KEY5) sent=43;                
                uart_send(0x90);
                uart_send(sent);
                uart_send(127);
            }            
        }
        if(sent) {
            uart_send(0x90);
            uart_send(sent);
            uart_send(0);
            sent = 0;
        }
        
    }
}
