
#if 0

#include <xc.h>
#include "mn.h"

typedef struct {
    FWORD cookie;
} APP_CONFIG;
const FWORD APP_COOKIE = 0x1601; // 14 bits max


APP_CONFIG g_cfg;

void flash(int count) {
    for(int i=0; i<count; ++i) {
        mn_set_left(1);
        __delay_ms(100);
        mn_set_left(0);
        __delay_ms(200);
    }
}
////////////////////////////////////////////////////////////////////////////////
void app_key_event(byte event, byte keys) {
    if(event == EV_KEY_DOWN) {
        switch(keys) {
            case KEY_1:
                //flash(1);
                break;
            case KEY_2:
                //flash(2);
                break;
            case KEY_3:
                //flash(3);
                break;
            case KEY_4:
                //flash(4);
                break;
            case KEY_5:
                //flash(5);
                break;
        }
    }
    else if(event == EV_KEY_HOLD) {
        switch(keys) {        
            case KEY_5: 
                mn_blink_left();
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
    mn_send_midi_msg(status, num_params, param1, param2);
    /*
    switch(status & 0xF0) {
        case MIDI_STATUS_NOTE_ON:
        case MIDI_STATUS_NOTE_OFF:
        default:
            // pass thru
            mn_send_midi_msg(status, num_params, param1, param2);
            break;
    }*/
}
////////////////////////////////////////////////////////////////////////////////
int qq=0;
void app_tick() {
#if 0    
    if(!(qq&0xF)) {
        mn_send_midi_msg(MIDI_STATUS_NOTE_ON, 2, qq>>4, 127);        
    }
    qq++;
    qq&=0x7FF;
#endif     
}
////////////////////////////////////////////////////////////////////////////////
void app_init() {
    mn_blink_right();
}
////////////////////////////////////////////////////////////////////////////////
void app_run() {   
}
#endif

