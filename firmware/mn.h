typedef unsigned char byte;
typedef unsigned short FWORD;   // Flash memory word is 14 bits
#define PRIVATE static
#define PUBLIC

#define P_LED1 LATAbits.LATA5
#define P_LED2 LATCbits.LATC0
#define P_KEY1 PORTCbits.RC3
#define P_KEY2 PORTAbits.RA4
#define P_KEY3 PORTCbits.RC6
#define P_KEY4 PORTCbits.RC7
#define P_KEY5 PORTBbits.RB7

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




const byte NO_NOTE = 0xff;
const byte NO_CHAN = 0xff;


#define MIDI_MTC_QTR_FRAME          0xf1
#define MIDI_SPP                    0xf2
#define MIDI_SONG_SELECT            0xf3 
#define MIDI_SYNCH_TICK             0xf8
#define MIDI_SYNCH_START            0xfa
#define MIDI_SYNCH_CONTINUE         0xfb
#define MIDI_SYNCH_STOP             0xfc
#define MIDI_SYSEX_BEGIN            0xf0
#define MIDI_SYSEX_END              0xf7
#define MIDI_STATUS_NOTE_OFF        0x80    //  Note-off: 2 params (note, velocity)
#define MIDI_STATUS_NOTE_ON         0x90    //  Note-on: 2 params (note, velocity)
#define MIDI_STATUS_POLY_AFTERTOUCH 0xA0    //  Polyphonic aftertouch: 2 params (note, touch) 
#define MIDI_STATUS_CC              0xB0    //  Continuous controller: 2 params (cc#, value)
#define MIDI_STATUS_PATCH_CHANGE    0xC0    //  Patch change: 1 param (instrument)
#define MIDI_STATUS_CHAN_PRESSURE   0xD0    //  Channel Pressure: 1  pressure  
#define MIDI_STATUS_PITCH_BEND      0xE0    //  Pitch bend: 2 params (lsb7, msb7)  

#define _XTAL_FREQ 32000000

void __interrupt() ISR();

typedef struct {
    void (*app_key_event)(byte event, byte keys);
    void (*app_midi_realtime)(byte data);
    void (*app_midi_msg)(byte status, byte num_params, byte param1, byte param2);
    void (*app_tick)(void);
    void (*app_run)(void);
} MNApp;


///////////////////////////////////////////////////
extern MNApp g_app;

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

///////////////////////////////////////////////////
void app_init_chord_strum(void);
void app_init_da_chord(void);
