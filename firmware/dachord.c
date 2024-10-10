////////////////////////////////////////////////////////////////////////////////
// MIDI NUBBIN : DA CHORD
// Chord maker
//
// Button assignments
// [OCTAVES-1]      [OCTAVES+1]
// [MINOR]          [MAJOR]         [ENABLE]
//
// [ENABLE] toggles the device on and off 
// Long press of [ENABLE] performs a reset
//
// The MIDI channel and root note are "learned" from the first NOTE ON 
// message that is received following power up or reset. Only notes on this 
// channel are manipulated. All other MIDI is passed through unchanged
//

#if 1
#include <xc.h>
#include <string.h>
#include "mn.h"


const byte NO_NOTE = 0xff;
byte g_note_status[128];
byte g_chord_note;

enum {
    CH_NONE,
    CH_MAJ,
    CH_MIN,
    CH_DIM
};

typedef struct {
    byte root;
    byte type;
} CHORD;
const CHORD MAJ_CHORD[12] = {
{0, CH_MAJ}, //C
{1, CH_MAJ}, //C#
{2, CH_MIN}, //D
{3, CH_MAJ}, //D#
{4, CH_MIN}, //E
{5, CH_MAJ}, //F
{6, CH_MAJ}, //F#
{7, CH_MAJ}, //G
{8, CH_MAJ}, //G#
{9, CH_MIN}, //A
{10, CH_MAJ}, //A#
{11, CH_DIM}, //B         
};


////////////////////////////////////////////////////////////////////////////////
void chord_note_on(byte chan, byte note, byte vel) {
    if(note < 128) {
        mn_send_midi_msg(MIDI_STATUS_NOTE_ON|chan, 2, note, vel);    
        g_note_status[note] = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////
void chord_maj(byte chan, byte root, byte vel) {
    chord_note_on(chan, root, vel);
    chord_note_on(chan, root+4, vel);
    chord_note_on(chan, root+7, vel);
}
void chord_min(byte chan, byte root, byte vel) {
    chord_note_on(chan, root, vel);
    chord_note_on(chan, root+3, vel);
    chord_note_on(chan, root+7, vel);
}
void chord_dim(byte chan, byte root, byte vel) {
    chord_note_on(chan, root, vel);
    chord_note_on(chan, root+3, vel);
    chord_note_on(chan, root+6, vel);
}

////////////////////////////////////////////////////////////////////////////////
// note = the note the user has played
// scale_root = tthe 
void chord_on(byte chan, byte note, byte scale_root, const CHORD *scale, byte vel) {

    // based on the scale root note (0-11 for C through B) and the requested
    // midi note, work out where the MIDI note is in the scale. This determines
    // the chord that we'll play
    int note_in_scale = (note%12)-scale_root;
    if(note_in_scale<0) {
        note_in_scale += 12;
    }    
    const CHORD *chord = &scale[note_in_scale];
    
    // work out the note (with octave) at the start of the scale
    // and the note on which the chord is rooted
    byte start_of_scale = 12 * (note/12) + scale_root;       
    byte chord_root = start_of_scale + chord->root;   
    switch(chord->type) {
        case CH_MAJ:
            chord_maj(chan, chord_root, vel);
            break;
        case CH_MIN:
            chord_min(chan, chord_root, vel);
            break;
        case CH_DIM:        
            chord_dim(chan, chord_root, vel);
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////
void chord_off(byte chan) {
    for(byte i=0; i<128; ++i) {
        if(g_note_status[i]) {
            mn_send_midi_msg(MIDI_STATUS_NOTE_ON|chan, 2, i, 0);
            g_note_status[i] = 0;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void on_note_on(byte chan, byte note, byte vel) {
    if(g_chord_note != NO_NOTE && note != g_chord_note) {
        chord_off(chan);
    }
    g_chord_note = note;
    chord_on(chan, note, 0, MAJ_CHORD, vel);
}
////////////////////////////////////////////////////////////////////////////////
void on_note_off(byte chan, byte note) {
    if(note == g_chord_note) {
        chord_off(chan);
        g_chord_note = NO_NOTE;
    }
}
////////////////////////////////////////////////////////////////////////////////
void app_key_event(byte event, byte keys) {
    if(event == EV_KEY_DOWN) {
        switch(keys) {
            case KEY_1: // down octave
                break;
            case KEY_2: // up octave
                break;
            case KEY_3: // 
                break;
            case KEY_4: // up semitone
                break;
            case KEY_5: // toggle on/off
                break;
        }
    }
    else if(event == EV_KEY_HOLD) {
        switch(keys) {        
            case KEY_5: // reset
                //reset();
                mn_blink_right();
                break;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void app_midi_realtime(byte data) {
    mn_send_midi_realtime(data);    
}
////////////////////////////////////////////////////////////////////////////////
void app_midi_msg(byte status, byte num_params, byte param1, byte param2) {
    switch(status & 0xF0) {
        case MIDI_STATUS_NOTE_ON:
            if(param2) {
                on_note_on(status&0x0F, param1, param2);
                break;
            }
            // else fall through to next case
        case MIDI_STATUS_NOTE_OFF:
            on_note_off(status&0x0F, param1);
            break;
        default:
            mn_send_midi_msg(status, num_params, param1, param2);
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////
void app_tick() {
}
////////////////////////////////////////////////////////////////////////////////
void app_init() {
    memset(g_note_status, 0, sizeof g_note_status);
    g_chord_note = NO_NOTE;
}
////////////////////////////////////////////////////////////////////////////////
void app_run() {   
}

#endif
