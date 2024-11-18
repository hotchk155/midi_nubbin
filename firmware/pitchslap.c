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

#if 1
#include <xc.h>
#include "mn.h"
#include "mn_utils.h"

const int MAX_TRANSPOSE = 36;
const int MIN_TRANSPOSE = -36;

typedef struct {
    int transpose;
} APP_STATE;
PRIVATE APP_STATE g_st;

////////////////////////////////////////////////////////////////////////////////
PRIVATE void inc_transpose(int delta) {
    mn_note_array_off();    
    g_st.transpose += delta;
    if(g_st.transpose < MIN_TRANSPOSE) {
        g_st.transpose  = MIN_TRANSPOSE;
    }
    if(g_st.transpose > MAX_TRANSPOSE) {
        g_st.transpose  = MAX_TRANSPOSE;
    }    
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void on_note(byte note, byte vel) {
    int new_note = note + g_st.transpose;
    if(new_note >= 0 && new_note < 128) {
        mn_send_midi_msg(MIDI_STATUS_NOTE_ON|g_mn.chan, 2, (byte)new_note, vel);
        if(vel) {
            mn_add_note_to_array((byte)new_note);
            mn_blink_right();
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void reset() {
    mn_note_array_off();
    mn_utils_reset();
    g_st.transpose = 0;
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void app_key_event(byte event, byte keys) {
    if(event == EV_KEY_DOWN) {
        switch(keys) {
            case KEY_1: // down octave
                inc_transpose(-12);
                break;
            case KEY_2: // up octave
                inc_transpose(+12);
                break;
            case KEY_3: // 
                inc_transpose(-1);
                break;
            case KEY_4: // up semitone
                inc_transpose(+1);
                break;
            case KEY_5: // toggle on/off
                g_mn.enabled = !g_mn.enabled;
                mn_note_array_off();    // mute all playing notes on the channel 
                mn_clear_note_array();                
                break;
        }
    }
    else if(event == EV_KEY_HOLD) {
        switch(keys) {        
            case KEY_5: // reset
                reset();
                break;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void app_midi_msg(byte status, byte num_params, byte param1, byte param2) {
    if(g_mn.enabled) {
        mn_app_std_midi_msg(status, num_params, param1, param2);
        if(status == (MIDI_STATUS_NOTE_ON|g_mn.chan)) {            
            on_note(param1, param2);
            return;
        }
        if(status == (MIDI_STATUS_NOTE_OFF|g_mn.chan)) {
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
PUBLIC void app_init_pitchslap() {
    g_app.app_key_event = app_key_event;
    g_app.app_midi_msg = app_midi_msg;    
    g_app.app_tick = mn_app_std_leds;
    reset();
}

#endif