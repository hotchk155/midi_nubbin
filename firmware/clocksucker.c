#include <xc.h>
#include "mn.h"
#if 0
typedef struct {
    FWORD cookie;
    FWORD pass_transport;
} APP_CONFIG;
const FWORD APP_COOKIE = 0x15a1; // 14 bits max

int g_is_running;
int g_tick_count;
int g_ticks_to_drop;
APP_CONFIG g_cfg;

////////////////////////////////////////////////////////////////////////////////
void load_config() {
    // set defaults
    g_cfg.pass_transport = 0;
    
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
void send_tick_out() {
    mn_send_midi_realtime(MIDI_SYNCH_TICK);
    if(!g_tick_count) {
        mn_blink_right();
    }
    if(++g_tick_count >= 6) {
        g_tick_count = 0;
    }
}
////////////////////////////////////////////////////////////////////////////////
void on_start() {
    g_is_running = 1;
    mn_set_left(1);
    g_tick_count = 0;
    g_ticks_to_drop = 0;
    mn_send_midi_realtime(MIDI_SYNCH_START);    
}
////////////////////////////////////////////////////////////////////////////////
void on_stop() {
    g_is_running = 0;
    mn_set_left(0);
    mn_send_midi_realtime(MIDI_SYNCH_STOP);
}
////////////////////////////////////////////////////////////////////////////////
void on_continue() {
    g_is_running = 1;
    mn_set_left(1);
    mn_send_midi_realtime(MIDI_SYNCH_CONTINUE);    
}
////////////////////////////////////////////////////////////////////////////////
void app_key_event(byte event, byte keys) {
    if(event == EV_KEY_DOWN) {
        switch(keys) {
            case KEY_1: // drop a tick
                ++g_ticks_to_drop;
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
            if(g_ticks_to_drop > 0) {
                --g_ticks_to_drop;
            }
            else {
                send_tick_out();
            }
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////
void app_midi_msg(byte status, byte num_params, byte param1, byte param2) {
}
////////////////////////////////////////////////////////////////////////////////
void app_tick() {
}
////////////////////////////////////////////////////////////////////////////////
void app_init() {
    g_is_running = 1;
    mn_set_left(1);
    g_tick_count = 0;
    g_ticks_to_drop = 0;    
}
////////////////////////////////////////////////////////////////////////////////
void app_run() {   
}
#endif