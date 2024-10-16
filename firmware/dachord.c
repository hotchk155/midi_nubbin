////////////////////////////////////////////////////////////////////////////////
// MIDI NUBBIN : DA CHORD
// Chord maker with strumming via the pitch bend wheel

#if 1
#include <xc.h>
#include <string.h>
#include "mn.h"
#include "mn_utils.h"

typedef struct {
    byte chord_vel;
    byte chord_root;    
} APP_STATE;
PRIVATE APP_STATE g_st;

////////////////////////////////////////////////////////////////////////////////
PRIVATE void on_note(byte note, byte vel) {
    
    // get the new root note based on whether this is a note on
    // or a note off event
    byte root = vel? mn_push_note(note) : mn_pop_note(note);
    
    // is there any change to chord currently playing?
    if(root != g_st.chord_root) {        
               
        // stop any notes of the currently playing chord
        mn_note_array_off();
        mn_clear_note_array();
        
        // if there is a new chord to play
        g_st.chord_root = root;
        if(g_st.chord_root != NO_NOTE) {
            
            if(vel) {
               g_st.chord_vel = vel;// if its a note on event then remember the velocity
            }
            
            // build the chord
            byte triad[3]; 
            mn_build_triad(g_st.chord_root, triad);        
            mn_add_note_to_array(triad[0]);
            mn_add_note_to_array(triad[1]);
            mn_add_note_to_array(triad[2]);
            mn_add_note_to_array(triad[0]+12);
            mn_note_array_on(g_st.chord_vel);
            mn_blink_right();
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void reset() {
    mn_note_array_off();
    mn_pop_all_notes();
    g_mn.enabled = 1;
    g_mn.chan = NO_CHAN;
    g_mn.split_point = 0;
    g_mn.apply_above_split = 1;
    g_mn.scale = g_maj_scale;                
    g_mn.scale_root = NOTE_C;
    g_st.chord_vel = 0;
    g_st.chord_root = NO_NOTE;
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void app_key_event(byte event, byte keys) {
    if(event == EV_KEY_DOWN) {
        switch(keys) {
            case KEY_1: 
                g_mn.scale = g_min_scale;      
                break;
            case KEY_2: 
                g_mn.scale = g_maj_scale;                
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
////////////////////////////////////////////////////////////////////////////////
PRIVATE void app_midi_msg(byte status, byte num_params, byte param1, byte param2) {    
    if(g_mn.enabled) {
        mn_app_std_midi_msg(status, num_params, param1, param2);
        if(status == (MIDI_STATUS_NOTE_ON|g_mn.chan) && mn_app_apply_to_note(param1)) {            
            on_note(param1, param2);
            return;
        }
        if(status == (MIDI_STATUS_NOTE_OFF|g_mn.chan) && mn_app_apply_to_note(param1)) {
            on_note(param1, 0);
            return;
        }
    }
    else {
        // track note messages on the selected channel so that ON notes can be 
        // muted when the effect is turned on
        mn_note_array_track_note_msg(status, num_params, param1, param2);
    }
    mn_send_midi_msg(status, num_params, param1, param2);
}
////////////////////////////////////////////////////////////////////////////////
PUBLIC void app_init_da_chord() {
    g_app.app_key_event = app_key_event;
    g_app.app_midi_msg = app_midi_msg;    
    g_app.app_tick = mn_app_std_leds;
    reset();
}
#endif

