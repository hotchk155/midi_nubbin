////////////////////////////////////////////////////////////////////////////////
// MIDI NUBBIN : CHORD STRUM

#if 1
#include <xc.h>
#include <string.h>
#include "mn.h"
#include "mn_utils.h"

enum {
    STRUM_NONE,
    STRUM_UP,
    STRUM_DOWN
};
typedef struct {
    byte current_pos;
    byte chord_vel;
    byte chord_root;
    byte strum_pos;
    byte strum_dir;
    byte strum_period;
    byte tick_count;
} APP_STATE;
PRIVATE APP_STATE g_st;

///////////////////////////////////////////////////////////////////////////////
// pos = 0-127
PRIVATE byte map_strum(byte strum_pos) {  
    int pos = g_st.chord_root - 36 + (72 * strum_pos)/128;
    if(pos < 0) {
        return 0;
    }
    if(pos > 127) {
        return 127;
    }    
    return(byte)pos;
}

PRIVATE void strum_to(byte strum_pos) {      
    byte pos = map_strum(strum_pos);
    while(g_st.current_pos != pos) {
        mn_note_array_note_on(g_st.current_pos, g_st.chord_vel);
        if(g_st.current_pos < pos) {
            ++g_st.current_pos;
        }
        else {
            --g_st.current_pos;
        }
    }
}
PRIVATE void strum_begin() {  
    g_st.current_pos = g_st.chord_root;
    mn_note_array_note_on(g_st.chord_root, g_st.chord_vel);
    g_st.strum_pos = g_st.current_pos;     
    g_st.strum_dir = STRUM_UP;
    g_st.tick_count = 0;
}
PRIVATE void strum_end() {  
    g_st.strum_dir = STRUM_NONE;
}


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
            mn_build_triad(g_st.chord_root%12, triad);        
            for(int i=0; i<3; ++i) {
                while(triad[i]<128) {
                    mn_add_note_to_array(triad[i]);
                    triad[i] += 12;
                }
            }
            strum_begin();            
        }
        else {
            strum_end();
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void reset() {
    mn_note_array_off();
    mn_utils_reset();
    g_st.current_pos = 64;
    g_st.chord_vel = 0;
    g_st.chord_root = NO_NOTE;
    g_st.strum_dir = STRUM_NONE;    
    g_st.strum_period = 10;
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
////////////////////////////////////////////////////////////////////////////////
PRIVATE void app_tick() {
    mn_app_std_leds();
    
    if(g_st.strum_dir != STRUM_NONE) {
        if(++g_st.tick_count >= g_st.strum_period) {
            g_st.tick_count = 0;
            switch(g_st.strum_dir) {
                case STRUM_UP:
                    if(g_st.strum_pos >= 127) {
                        g_st.strum_pos = 126;
                        g_st.strum_dir = STRUM_DOWN;
                    }
                    else {
                        ++g_st.strum_pos;
                    }
                    break;
                case STRUM_DOWN:
                    if(!g_st.strum_pos) {
                        g_st.strum_pos = 1;
                        g_st.strum_dir = STRUM_UP;
                    }
                    else {
                        --g_st.strum_pos;
                    }
                    break;
            }
            strum_to(g_st.strum_pos);
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
        if(status == (MIDI_STATUS_CC|g_mn.chan) && param1 == 1) {
            if(g_st.strum_dir == STRUM_NONE) {
                strum_to(param2);
            }
            g_st.strum_period = (127-param2)/4;
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
PUBLIC void app_init_chord_strum() {
    g_app.app_key_event = app_key_event;
    g_app.app_midi_msg = app_midi_msg;    
    g_app.app_tick = app_tick;
    reset();
}

#endif

