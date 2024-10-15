#include <xc.h>
#include <string.h>
#include "mn.h"
#include "mn_utils.h"

MN_STD_STATE g_mn;

// general purpose array of 128 bytes 
PRIVATE byte g_note_array[128];
        
#define SZ_NOTE_STACK  8
PRIVATE byte g_note_stack_size;
PRIVATE byte g_note_stack[SZ_NOTE_STACK];// highest note at position 0

const byte g_maj_scale[12] = {
    CHORD_MAJ, //C
    CHORD_MAJ, //C#
    CHORD_MIN, //D
    CHORD_MAJ, //D#
    CHORD_MIN, //E
    CHORD_MAJ, //F
    CHORD_MAJ, //F#
    CHORD_MAJ, //G
    CHORD_MAJ, //G#
    CHORD_MIN, //A
    CHORD_MAJ, //A#
    CHORD_DIM, //B         
};
const byte g_min_scale[12] = {
    CHORD_MIN, //A
    CHORD_MIN, //A#
    CHORD_DIM, //B
    CHORD_MAJ, //C
    CHORD_MAJ, //C#
    CHORD_MIN, //D
    CHORD_MIN, //D#
    CHORD_MIN, //E
    CHORD_MAJ, //F
    CHORD_MAJ, //F#
    CHORD_MAJ, //G
    CHORD_MAJ, //G#         
};


////////////////////////////////////////////////////////////////////////////////
void mn_clear_note_array() {
    memset(g_note_array, 0, sizeof g_note_array);
}
////////////////////////////////////////////////////////////////////////////////
void mn_add_note_to_array(byte note) {
    if(note<128) {
        g_note_array[note] = 1;
    }
}
////////////////////////////////////////////////////////////////////////////////
void mn_note_array_on(byte vel) {
    for(byte i = 0; i<128; ++i) {
        if(g_note_array[i]) {
            mn_send_midi_msg(MIDI_STATUS_NOTE_ON|g_mn.chan, 2, i, vel);    
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void mn_note_array_off() {
    for(byte i = 0; i<128; ++i) {
        if(g_note_array[i]) {
            mn_send_midi_msg(MIDI_STATUS_NOTE_ON|g_mn.chan, 2, i, 0);    
        }
    }
}

void _dump_stack() {
    for(byte i=0; i<g_note_stack_size; ++i) {
        mn_send_midi_msg(MIDI_STATUS_NOTE_ON|1, 2, i, g_note_stack[i]);    
    }
}

////////////////////////////////////////////////////////////////////////////////
byte mn_push_note(byte note) {
    int pos;
    for(pos=0; pos<g_note_stack_size; ++pos) {
        if(g_note_stack[pos] == note) {
//_dump_stack();
            return g_note_stack[0]; // already in the list!
        }
        if(g_note_stack[pos] < note) {
            break; // found a note lower, so this is our insert point
        }
    }
    // increase length of list
    if(g_note_stack_size < SZ_NOTE_STACK) {
        ++g_note_stack_size;
    }    
    // shift lower notes down the list
    for(int i=g_note_stack_size-2; i>=pos; --i) {
        g_note_stack[i+1] = g_note_stack[i];
    }
    // add new note
    g_note_stack[pos] = note;
//_dump_stack();
    return g_note_stack[0];
}
////////////////////////////////////////////////////////////////////////////////
byte mn_pop_note(byte note) {
    // search for the note
    for(int i=0; i<g_note_stack_size; ++i) {
        // found it
        if(g_note_stack[i] == note) {
            // one fewer notes in the list
            --g_note_stack_size;
            // move all the following notes down one position
            for(int j=i; j<g_note_stack_size; ++j) {
                g_note_stack[j] = g_note_stack[j+1];
            }
            break;
        }
    }
//_dump_stack();
    return g_note_stack_size? g_note_stack[0] : NO_NOTE;
}
////////////////////////////////////////////////////////////////////////////////
void mn_pop_all_notes() {
    g_note_stack_size = 0;
}
////////////////////////////////////////////////////////////////////////////////
PRIVATE void chord_note(byte **dest, byte note) {
    **dest = (note<128)? note:NO_NOTE;
    ++(*dest);
}
void mn_build_triad(byte root, byte *dest) {    
    byte note_in_scale = (byte)(12 + root - g_mn.scale_root)%12;    
    switch(g_mn.scale[note_in_scale]) {
        case CHORD_MAJ:
            chord_note(&dest,root);
            chord_note(&dest,root+4);
            chord_note(&dest,root+7);
            break;
        case CHORD_MIN:
            chord_note(&dest,root);
            chord_note(&dest,root+3);
            chord_note(&dest,root+7);
            break;
        case CHORD_DIM:
            chord_note(&dest,root);
            chord_note(&dest,root+3);
            chord_note(&dest,root+6);
            break;
        default:
            chord_note(&dest,NO_NOTE);
            chord_note(&dest,NO_NOTE);
            chord_note(&dest,NO_NOTE);
            break;
    }
}
void mn_utils_init() {
    mn_pop_all_notes();
    mn_clear_note_array();
    g_mn.scale = g_maj_scale;
    g_mn.scale_root = NOTE_C;
    g_mn.chan = NO_CHAN;
    g_mn.enabled = 1;
    g_mn.split_point = 0;
    g_mn.apply_above_split = 1;
}


void mn_app_std_leds() {
    static byte ticker;
    if(!g_mn.enabled) {
        P_LED1 = 0;
    }
    else if(g_mn.chan == NO_CHAN || g_mn.scale_root == NO_NOTE || g_mn.split_point == NO_NOTE) {
        P_LED1 = !(ticker & 0x40);
        ++ticker;
    }
    else {
        P_LED1 = 1;
    }
}

void mn_app_std_midi_msg(byte status, byte num_params, byte param1, byte param2) {
    if(((status & 0xF0) == MIDI_STATUS_NOTE_ON) && param2) {       
        byte chan = status & 0x0F;
        if(g_mn.chan == NO_CHAN) {
            g_mn.chan = chan;
        }
        if(g_mn.scale_root == NO_NOTE && chan == g_mn.chan) {
            g_mn.scale_root = param1;
        }
        if(g_mn.split_point == NO_NOTE && chan == g_mn.chan) {
            g_mn.split_point = param1;
        }
    }
}
byte mn_app_apply_to_note(byte note) {
    if(g_mn.split_point == NO_NOTE) {
        return 1;
    }
    else if(g_mn.apply_above_split) {
        return (g_mn.split_point <= note);
    }
    else {
        return (g_mn.split_point > note);        
    }
}