
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

#define TIMER_0_INIT_SCALAR		5	// Timer 0 is an 8 bit timer counting at 250kHz
									// using this init scalar means that rollover


#define MIDI_SYNCH_TICK     	0xf8
#define MIDI_SYNCH_START    	0xfa
#define MIDI_SYNCH_CONTINUE 	0xfb
#define MIDI_SYNCH_STOP     	0xfc
#define MIDI_MTC_QTR_FRAME 		0xf1
#define MIDI_SPP 				0xf2
#define MIDI_SONG_SELECT 		0xf3 
#define MIDI_SYSEX_BEGIN     	0xf0
#define MIDI_SYSEX_END     		0xf7

extern byte timer_ticked = 0;
void mw_init();
void mw_run();

extern void on_key(byte key);
extern byte on_clock(byte ch);
extern void on_note(byte status, byte *note);
