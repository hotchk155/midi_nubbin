#include <xc.h>
#include "mn.h"
#if 1
typedef struct {
    byte is_running;    
    byte tick_count;
    byte ticks_to_drop;
} APP_STATE;
PRIVATE APP_STATE g_st;

////////////////////////////////////////////////////////////////////////////////
void send_tick_out() {
    mn_send_midi_realtime(MIDI_SYNCH_TICK);
    if(!g_st.tick_count) {
        mn_blink_right();
    }
    if(++g_st.tick_count >= 6) {
        g_st.tick_count = 0;
    }
}
////////////////////////////////////////////////////////////////////////////////
void on_start() {
    g_st.is_running = 1;
    g_st.tick_count = 0;
    g_st.ticks_to_drop = 0;
    mn_send_midi_realtime(MIDI_SYNCH_START);    
    mn_set_left(1);
}
////////////////////////////////////////////////////////////////////////////////
void on_stop() {
    g_st.is_running = 0;
    mn_send_midi_realtime(MIDI_SYNCH_STOP);
    mn_set_left(0);
}
////////////////////////////////////////////////////////////////////////////////
void on_continue() {
    g_st.is_running = 1;
    mn_send_midi_realtime(MIDI_SYNCH_CONTINUE);    
    mn_set_left(1);
}
////////////////////////////////////////////////////////////////////////////////
void app_key_event(byte event, byte keys) {
    if(event == EV_KEY_DOWN) {
        switch(keys) {
            case KEY_1: // drop a tick
                ++g_st.ticks_to_drop;
                break;
            case KEY_2: // add a tick
                send_tick_out();
                break;
            case KEY_3: // start
                on_start();
                break;
            case KEY_4: // stop
                on_stop();
                break;
            case KEY_5: // continue
                on_continue();
                break;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void app_midi_realtime(byte data) {
    switch(data) {
        case MIDI_SYNCH_START:
            on_start();
            break;
        case MIDI_SYNCH_STOP:
            on_stop();
            break;
        case MIDI_SYNCH_CONTINUE:
            on_continue();
            break;
        case MIDI_SYNCH_TICK:
            if(g_st.ticks_to_drop > 0) {
                --g_st.ticks_to_drop;
            }
            else {
                send_tick_out();
            }
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////
void reset() {
    g_st.is_running = 1;
    g_st.tick_count = 0;
    g_st.ticks_to_drop = 0;    
    mn_set_left(1);
}

////////////////////////////////////////////////////////////////////////////////
PUBLIC void app_init_clocksucker() {
    g_app.app_key_event = app_key_event;
    g_app.app_midi_realtime = app_midi_realtime;    
    reset();
}

#endif