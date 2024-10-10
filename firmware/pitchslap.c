////////////////////////////////////////////////////////////////////////////////
// MIDI NUBBIN : PITCH SLAP
// MIDI note transposer. 
//
// Button assignments
// [DOWN OCTAVE]    [UP OCTAVE]
// [DOWN SEMITONE]  [UP SEMITONE]   [ENABLE]
//
// [ENABLE] toggles the device on and off 
// Long press of [ENABLE] performs a reset
//
// The MIDI channel is "learned" from the first NOTE ON message that is
// received following power up or reset. Only notes on this channel are 
// transposed. All other MIDI is passed through unchanged
//
// Device remembers notes that are playing so that they can be stopped
// after transposition

#if 0
#include <xc.h>
#include "mn.h"

typedef struct {
    FWORD cookie;
} APP_CONFIG;
const FWORD APP_COOKIE = 0x1501; // 14 bits max

const int MAX_TRANSPOSE = 36;
const int MIN_TRANSPOSE = -36;
const byte NO_CHAN = 0xff;
const byte NO_NOTE = 0xff;

byte g_enable;
int g_transpose;
byte g_chan;
byte g_transposed_notes[128];

APP_CONFIG g_cfg;

////////////////////////////////////////////////////////////////////////////////
void load_config() {
    
    APP_CONFIG cfg = {0};
    mn_saf_read((byte*)&g_cfg, sizeof g_cfg);
    if(cfg.cookie == APP_COOKIE) {
        g_cfg = cfg;
    }
}
////////////////////////////////////////////////////////////////////////////////
void save_config() {
    g_cfg.cookie = APP_COOKIE;
    mn_saf_write((byte*)&g_cfg, sizeof g_cfg);
}

////////////////////////////////////////////////////////////////////////////////
byte transpose_note(int note) {
    if(g_enable) {
        note += g_transpose;
        if(note < 0) {
            return NO_NOTE;
        }
        if(note > 127) {
            return NO_NOTE;
        }
    }
    return (byte)note;
}
////////////////////////////////////////////////////////////////////////////////
void on_note_on(byte chan, byte note, byte vel) {
    if(chan == g_chan || g_chan == NO_CHAN) {                
        // remember the channel
        g_chan = chan;        
        byte transposed_note = transpose_note(note);
        if(transposed_note != NO_NOTE) {
        
            // if the note is currently playing with a different transposition, 
            // we first need to silence the old transposed note
            if(g_transposed_notes[note] != NO_NOTE && g_transposed_notes[note] != transposed_note) {
                mn_send_midi_msg(MIDI_STATUS_NOTE_ON|g_chan, 2, g_transposed_notes[note], 0);            
                g_transposed_notes[note] = NO_NOTE;
            }        

            // now we need to start the new tranposed note playing
            mn_send_midi_msg(MIDI_STATUS_NOTE_ON|g_chan, 2, transposed_note, vel);            
            g_transposed_notes[note] = transposed_note;
            mn_blink_right();
        }
    }
    else {
        // otherwise simply pass note through
        mn_send_midi_msg(MIDI_STATUS_NOTE_ON|chan, 2, note, vel);
    }            
}
////////////////////////////////////////////////////////////////////////////////
void on_note_off(byte chan, byte note) {
    if(chan == g_chan) {                
        byte transposed_note = transpose_note(note);
        if(transposed_note != NO_NOTE) {
            mn_send_midi_msg(MIDI_STATUS_NOTE_ON|g_chan, 2, g_transposed_notes[note], 0);            
            g_transposed_notes[note] = NO_NOTE;
        }        
    }
    else {
        mn_send_midi_msg(MIDI_STATUS_NOTE_ON|chan, 2, note, 0);
    }            
}
////////////////////////////////////////////////////////////////////////////////
void reset() {
    if(g_chan != NO_CHAN) {
        for(int i=0; i<128; ++i) {
            if(g_transposed_notes[i] != NO_NOTE) {
                mn_send_midi_msg(MIDI_STATUS_NOTE_ON|g_chan, 2, g_transposed_notes[i], 0);            
                g_transposed_notes[i] = NO_NOTE;
            }            
        }
    }
    g_chan = NO_CHAN;
    g_transpose = 0;
}

////////////////////////////////////////////////////////////////////////////////
void app_key_event(byte event, byte keys) {
    if(event == EV_KEY_DOWN) {
        switch(keys) {
            case KEY_1: // down octave
                g_transpose -= 12;
                if(g_transpose < MIN_TRANSPOSE) {
                    g_transpose  = MIN_TRANSPOSE;
                }
                break;
            case KEY_2: // up octave
                g_transpose += 12;
                if(g_transpose > MAX_TRANSPOSE) {
                    g_transpose  = MAX_TRANSPOSE;
                }
                break;
            case KEY_3: // 
                if(--g_transpose < MIN_TRANSPOSE) {
                    g_transpose  = MIN_TRANSPOSE;
                }
                break;
            case KEY_4: // up semitone
                if(++g_transpose > MAX_TRANSPOSE) {
                    g_transpose  = MAX_TRANSPOSE;
                }
                break;
            case KEY_5: // toggle on/off
                g_enable = !g_enable;
                mn_set_left(g_enable);
                break;
        }
    }
    else if(event == EV_KEY_HOLD) {
        switch(keys) {        
            case KEY_5: // reset
                reset();
                mn_blink_right();
                break;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void app_midi_realtime(byte data) {
    // pass thru
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
            // pass thru
            mn_send_midi_msg(status, num_params, param1, param2);
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////
void app_tick() {
}
////////////////////////////////////////////////////////////////////////////////
void app_init() {
    g_chan = NO_CHAN;
    for(int i=0; i<128; ++i) {
        g_transposed_notes[i] = NO_NOTE;
    }
    g_enable = 1;
    g_transpose = 0;
    mn_set_left(1);
}
////////////////////////////////////////////////////////////////////////////////
void app_run() {   
}

#endif