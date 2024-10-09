typedef unsigned char byte;
typedef unsigned short FWORD;   // Flash memory word is 14 bits

enum {
    EV_NONE = 0,
    EV_KEY_DOWN, 
    EV_KEY_HOLD
};
enum {
    KEY_1 = (1<<0),
    KEY_2 = (1<<1),
    KEY_3 = (1<<2),
    KEY_4 = (1<<3),
    KEY_5 = (1<<4)
};


#define MIDI_MTC_QTR_FRAME 		0xf1
#define MIDI_SPP 				0xf2
#define MIDI_SONG_SELECT 		0xf3 
#define MIDI_SYNCH_TICK     	0xf8
#define MIDI_SYNCH_START    	0xfa
#define MIDI_SYNCH_CONTINUE 	0xfb
#define MIDI_SYNCH_STOP     	0xfc
#define MIDI_SYSEX_BEGIN     	0xf0
#define MIDI_SYSEX_END     		0xf7

#define MIDI_STATUS_NOTE_ON     0x90
#define MIDI_STATUS_NOTE_OFF    0x80

#define _XTAL_FREQ 32000000

void __interrupt() ISR();


///////////////////////////////////////////////////
void app_key_event(byte event, byte keys);
void app_midi_realtime(byte data);
void app_midi_msg(byte status, byte num_params, byte param1, byte param2);
void app_tick(void);
void app_init(void);
void app_run(void);

///////////////////////////////////////////////////
void mn_saf_read(byte *addr, size_t size);
void mn_saf_write(byte *addr, size_t size);
void mn_send_midi_msg(byte cmd, byte num_params, byte param1, byte param2 );
void mn_send_midi_realtime(byte msg);
void mn_set_left(byte status);
void mn_set_right(byte status);
void mn_blink_left(void);
void mn_blink_right(void);
void mn_init(void);
void mn_run(void);
