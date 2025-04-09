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

const int MAX_TRANSPOSE = 36;
const int MIN_TRANSPOSE = -36;

////////////////////////////////////////////////////////////////////////////////
PRIVATE void inc_transpose(int delta) {
    mn_chord_play(0);    
    int transpose = g_mn_cfg.transpose + delta;
    if(transpose < MIN_TRANSPOSE) {
        transpose  = MIN_TRANSPOSE;
    }
    if(transpose > MAX_TRANSPOSE) {
        transpose  = MAX_TRANSPOSE;
    }
    g_mn_cfg.transpose = (char)transpose;
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void on_note(byte note, byte vel) {
    if(g_mn_state.enabled) {
        int new_note = note + g_mn_cfg.transpose;
        if(new_note >= 0 && new_note < 128) {
            mn_chord_note((byte)new_note, vel);
            if(vel) {
                mn_blink_right();
            }
        }
    }
    else {
        // pass through the note message unchanged, but 
        // remember it in the chord so it can be stopped 
        // when the effect is engaged
        mn_chord_note(note, vel);
    }
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void reset() {
    mn_chord_play(0);
    mn_utils_reset();
    g_mn_cfg.transpose = 0;
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
                g_mn_state.enabled = !g_mn_state.enabled;
                mn_chord_play(0);    // mute all playing notes on the channel 
                mn_chord_init();                
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
    mn_std_handle_msg(status, num_params, param1, param2);    
    if((status & 0x0F) == g_mn_cfg.chan) {        
        switch(status & 0xF0) {
            case MIDI_STATUS_NOTE_ON:
                on_note(param1, param2);
                return;
            case MIDI_STATUS_NOTE_OFF:
                on_note(param1, 0);
                return;
        }       
    }
    mn_send_midi_msg(status, num_params, param1, param2);    
}
////////////////////////////////////////////////////////////////////////////////
PUBLIC void app_init_pitchslap() {
    g_app.app_key_event = app_key_event;
    g_app.app_midi_msg = app_midi_msg;    
    g_app.app_tick = mn_std_status_leds;
    reset();
}

#endif