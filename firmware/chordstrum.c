////////////////////////////////////////////////////////////////////////////////
// MIDI NUBBIN : CHORD STRUM

#if 1
#include <xc.h>
#include <string.h>
#include "mn.h"
#include "mn_utils.h"

PRIVATE byte g_current_pos;
PRIVATE byte g_chord_vel;
PRIVATE byte g_chord_root;

////////////////////////////////////////////////////////////////////////////////
PRIVATE void on_note(byte note, byte vel) {
    
    // get the new root note based on whether this is a note on
    // or a note off event
    byte root = vel? mn_push_note(note) : mn_pop_note(note);
    
    // is there any change to chord currently playing?
    if(root != g_chord_root) {        
               
        // stop any notes of the currently playing chord
        mn_note_array_off();
        mn_clear_note_array();
        
        // if there is a new chord to play
        g_chord_root = root;
        if(g_chord_root != NO_NOTE) {
            
            if(vel) {
                g_chord_vel = vel;// if its a note on event then remember the velocity
            }
            // build the chord
            byte triad[3]; 
            mn_build_triad(g_chord_root%12, triad);        
            for(int i=0; i<3; ++i) {
                while(triad[i]<128) {
                    mn_add_note_to_array(triad[i]);
                    triad[i] += 12;
                }
            }

        }
    }
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void reset() {
    mn_note_array_off();
    mn_clear_note_array();
    mn_pop_all_notes();
    g_mn.enabled = 1;
    g_mn.chan = NO_CHAN;
    g_mn.split_point = 0;
    g_mn.apply_above_split = 1;
    g_current_pos = 64;
    g_chord_vel = 0;
    g_chord_root = NO_NOTE;
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void app_key_event(byte event, byte keys) {
    if(event == EV_KEY_DOWN) {
        switch(keys) {
            case KEY_1: 
                g_mn.scale = g_min_scale;      
                g_mn.scale_root = NOTE_A;
                break;
            case KEY_2: 
                g_mn.scale = g_maj_scale;                
                g_mn.scale_root = NOTE_C;
                break;
            case KEY_3: 
                g_mn.apply_above_split = 0;
                break;
            case KEY_4: 
                g_mn.apply_above_split = 1;
                break;
            case KEY_5: // toggle on/off
                g_mn.enabled = !g_mn.enabled;
                mn_pop_all_notes();
                mn_note_array_off();
                mn_clear_note_array();
                break;
        }
    }
    else if(event == EV_KEY_HOLD) {
        switch(keys) {        
            case KEY_1: 
            case KEY_2: 
                g_mn.scale_root = NO_NOTE;
                break;
            case KEY_3: 
            case KEY_4:                
                g_mn.split_point = NO_NOTE;
                break;
            case KEY_5: // RESET
                reset();
                break;
        }
    }
}
///////////////////////////////////////////////////////////////////////////////
// pos = 0-127
PRIVATE void strum(byte pos) {    
    while(g_current_pos != pos) {
        mn_note_array_note_on(g_current_pos, g_chord_vel);
        if(g_current_pos < pos) {
            ++g_current_pos;
        }
        else {
            --g_current_pos;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void app_midi_msg(byte status, byte num_params, byte param1, byte param2) {    
    if(g_mn.enabled) {
        mn_app_std_midi_msg(status, num_params, param1, param2);
        if(status == (MIDI_STATUS_NOTE_ON|g_mn.chan) &&
            mn_app_apply_to_note(param1)) {            
            on_note(param1, param2);
            return;
        }
        if(status == (MIDI_STATUS_NOTE_OFF|g_mn.chan) &&
            mn_app_apply_to_note(param1)) {
            on_note(param1, 0);
            return;
        }
        //if(status == (MIDI_STATUS_PITCH_BEND|g_mn.chan)) {
        //    int value = ((int)param1) | (((int)param2)<<7);
        //    strum((byte)(value>>7));
        //    return;
        //}                   
        if(status == (MIDI_STATUS_CC|g_mn.chan) && param1 == 1) {
            strum(param2);
            return;
        }                   
    }
    mn_send_midi_msg(status, num_params, param1, param2);
}
////////////////////////////////////////////////////////////////////////////////
PUBLIC void app_init_chord_strum() {
    g_app.app_key_event = app_key_event;
    g_app.app_midi_msg = app_midi_msg;    
    g_app.app_tick = mn_app_std_leds;
    reset();
}

#endif

