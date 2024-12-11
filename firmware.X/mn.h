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

enum {
    CHORD_NONE,
    CHORD_MAJ,
    CHORD_MIN,
    CHORD_DIM
};

enum {
    NOTE_C,
    NOTE_C_SHARP,
    NOTE_D,
    NOTE_D_SHARP,
    NOTE_E,
    NOTE_F,
    NOTE_F_SHARP,
    NOTE_G,
    NOTE_G_SHARP,
    NOTE_A,
    NOTE_A_SHARP,
    NOTE_B
};

enum {
    MN_APP_CHORD,
    MN_APP_CHORD_STRUM,
    MN_APP_PITCH
};

enum {
    MN_SCALE_MAJOR,
    MN_SCALE_MINOR
};
typedef struct {
    byte chan;    
    byte scale_root;
    byte scale_type;
    byte split_point;
    byte apply_above_split;
} MN_CFG;
typedef struct {
    byte app_type;
    byte enabled; 
} MN_STATE;

extern MN_CFG g_mn_cfg;
extern MN_STATE g_mn_state;
extern const byte g_maj_scale[12];
extern const byte g_min_scale[12];


void mn_utils_reset(void);
byte mn_push_note(byte note);
byte mn_pop_note(byte note);
void mn_pop_all_notes(void);
void mn_build_triad(byte root, byte *dest);
void mn_clear_note_array(void);
void mn_add_note_to_array(byte note);
void mn_note_array_on(byte vel);
void mn_note_array_off(void);
void mn_note_array_note_on(byte index, byte vel);
void mn_note_array_track_note_msg(byte status, byte num_params, byte note, byte vel);
void mn_app_std_leds(void);
void mn_app_std_midi_msg(byte status, byte num_params, byte param1, byte param2);
byte mn_app_apply_to_note(byte note);
inline const byte *mn_scale(byte type);
void mn_app_init();
void mn_save_settings();

///////////////////////////////////////////////////
void app_init_chord_strum(void);
void app_init_da_chord(void);

