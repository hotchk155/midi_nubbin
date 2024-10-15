////////////////////////////////////////////////////////////////////////////////
// MIDI NUBBIN : CHORD STRUM
// Chord maker with strumming via the pitch bend wheel
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

#if 0
#include <xc.h>
#include <string.h>
#include "mn.h"
#include "mn_utils.h"

byte g_current_pos;
byte g_chord_vel;
byte g_chord_root;

////////////////////////////////////////////////////////////////////////////////
// chan / note = the note the user has played
// scale_root = the root note the scale
// scale = definition of chords in the scale
// vel = 0 to stop the chord
PRIVATE void select_chord(byte note) {
    byte triad[3]; 
    mn_build_triad(note, triad);        
    mn_clear_note_array();
    for(int i=0; i<3; ++i) {
        while(triad[i]<128) {
            g_note_array[triad[i]] = 1;
            triad[i] += 12;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
PRIVATE void on_note(byte note, byte vel) {
    
    // get the new root note based on whether this is a note on
    // or a note off event
    byte root = vel? mn_push_note(note) : mn_pop_note(note);
    
    // is there any change to chord currently playing?
    if(root != g_chord_root) {        
               
        // stop any notes of the currently playing chord
        mn_note_array_off();
        
        // if there is a new chord to play
        g_chord_root = root;
        if(g_chord_root != NO_NOTE) {
            // if its a note on event then remember the velocity
            if(vel) {
                g_chord_vel = vel;
            }
            // and switch to the new chord
            select_chord(g_chord_root, NOTE_C, g_maj_scale);
            mn_blink_right();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// pitch bend is 0-16384
PRIVATE void strum(int pos) {    
    if(g_chan != NO_CHAN) {
        pos >>= 7; // convert 14 bits to 7 bits
        while(pos != g_current_pos) {
            if(g_note_array[g_current_pos]) {
                mn_send_midi_msg(MIDI_STATUS_NOTE_ON|g_chan, 2, (byte)pos, g_chord_vel);                
            }
            g_current_pos = (g_current_pos < pos)? (g_current_pos+1) : (g_current_pos-1);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
PRIVATE void reset() {
    g_chan = NO_CHAN;
    g_chord_root = NO_NOTE;
    g_chord_vel = 0;
    g_current_pos = 64;
    g_enabled = 1;
    mn_set_left(1);
}

////////////////////////////////////////////////////////////////////////////////
PRIVATE void app_key_event(byte event, byte keys) {
    if(event == EV_KEY_DOWN) {
        switch(keys) {
            case KEY_1: 
                break;
            case KEY_2: 
                break;
            case KEY_3: 
                break;
            case KEY_4: 
                break;
            case KEY_5: // toggle on/off
                g_enabled = !g_enabled;
                mn_set_left(g_enabled);
                mn_note_array_off();
                break;
        }
    }
    else if(event == EV_KEY_HOLD) {
        switch(keys) {        
            case KEY_5: // reset
                mn_blink_right();
                mn_note_array_off();
                reset();
                break;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void app_midi_msg(byte status, byte num_params, byte param1, byte param2) {    
    byte pass = 1;
    if(g_enabled && g_chan == (status & 0x0F)) {
        switch(status & 0xF0) {
            case MIDI_STATUS_NOTE_ON:
                on_note(param1, param2);
                pass = 0;
                break;
            case MIDI_STATUS_PITCH_BEND:
                strum((int)param1 | ((int)param2<<7));
                pass = 0;
                break;
        }
    }
    if(pass) {
        mn_send_midi_msg(status, num_params, param1, param2);
    }
}
////////////////////////////////////////////////////////////////////////////////
PUBLIC void app_init_chordstrum() {
    g_app.app_key_event = app_key_event;
    g_app.app_midi_msg = app_midi_msg;    
    mn_pop_all_notes();
    reset();
}

#endif

