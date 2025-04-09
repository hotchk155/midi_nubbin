#include <xc.h>
#include <string.h>
#include "mn.h"

MN_CFG g_mn_cfg;
MN_STATE g_mn_state;

PRIVATE byte g_chord[128];

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
void mn_save_settings() {
    //TODO
}
////////////////////////////////////////////////////////////////////////////////
inline const byte *mn_scale(byte type) {
    switch(type) {
        case MN_SCALE_MINOR:
            return g_min_scale;
        default: //case MN_SCALE_MAJOR:
            return g_maj_scale;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// MIDI functions
// 
// Functions to directly send MIDI messages
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void mn_midi_note(byte note, byte vel) {
    if(g_mn_cfg.chan != NO_CHAN) {
        mn_send_midi_msg(MIDI_STATUS_NOTE_ON|g_mn_cfg.chan, 2, note, vel);        
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Helper functions to maintain a "chord" of 0-127 notes. 
// 
// Each element in the chord array has one of the following values:
// 
// - NO_NOTE    The note is not part of the chord
// - 0          The note is part of the chord but is not playing
// - 1..127     The note is part of the chord and is playing with this velocity
// 
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// initialise the chord array
void mn_chord_init() {
    for(int i=0; i<128; ++i) {
        g_chord[i] = NO_NOTE;
    }
}
////////////////////////////////////////////////////////////////////////////////
// indicate that a note is valid in the chord, without sending MIDI message
void mn_chord_add(byte note) {
    if(note<128) {
        g_chord[note] = 0;
    }
}
////////////////////////////////////////////////////////////////////////////////
// start a note playing and remember it is playing
void mn_chord_note(byte note, byte vel) {
    if(note<128) {
        mn_midi_note(note, vel);
        g_chord[note] = vel;
    }
}
////////////////////////////////////////////////////////////////////////////////
// play all notes of the chord with specified velocity
void mn_chord_play(byte vel) {
    for(byte i = 0; i<128; ++i) {
        // check note 
        if(g_chord[i] != NO_NOTE) {            
            mn_chord_note(i, vel);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Helper functions to maintain a "stack" of input notes which are currently
// held down by the user. This is sorted in highest note order
// 
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// diagnostic function
void _dump_stack() {
    for(byte i=0; i<g_note_stack_size; ++i) {
        mn_send_midi_msg(MIDI_STATUS_NOTE_ON|1, 2, i, g_note_stack[i]);    
    }
}

////////////////////////////////////////////////////////////////////////////////
// add a note into the stack and return the highest held note
byte mn_note_stack_push(byte note) {
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
// remove a note from the stack and return the highest remaining note
byte mn_note_stack_pop(byte note) {
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
// clear the note stack
void mn_note_stack_init() {
    g_note_stack_size = 0;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Chord and scale helpers
// 
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
PRIVATE void triad_note(byte **dest, byte note) {
    **dest = (note<128)? note:NO_NOTE;
    ++(*dest);
}

////////////////////////////////////////////////////////////////////////////////
// build a specified 3 note triad on specified root note
void mn_triad(byte root, byte chord_type, byte *dest) {    
    switch(chord_type) {
        case CHORD_MAJ:
            triad_note(&dest,root);
            triad_note(&dest,root+4);
            triad_note(&dest,root+7);
            break;
        case CHORD_MIN:
            triad_note(&dest,root);
            triad_note(&dest,root+3);
            triad_note(&dest,root+7);
            break;
        case CHORD_DIM:
            triad_note(&dest,root);
            triad_note(&dest,root+3);
            triad_note(&dest,root+6);
            break;
        default:
            triad_note(&dest,NO_NOTE);
            triad_note(&dest,NO_NOTE);
            triad_note(&dest,NO_NOTE);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// given the current scale, determine the chord type for the given root note
// and build the triad
void mn_triad_for_scale(byte root, byte *dest) {    
    byte note_in_scale = (byte)(12 + root - g_mn_cfg.scale_root)%12;    
    const byte *scale = mn_scale(g_mn_cfg.scale_type);
    mn_triad(root, scale[note_in_scale], dest);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
void mn_utils_reset() {
    mn_note_stack_init();
    mn_chord_init();
    g_mn_cfg.scale_type = MN_SCALE_MAJOR;
    g_mn_cfg.scale_root = NOTE_C;
    g_mn_cfg.chan = NO_CHAN;
    g_mn_state.enabled = 1;
    g_mn_cfg.split_point = 0;
    g_mn_cfg.apply_above_split = 1;
}
////////////////////////////////////////////////////////////////////////////////
void mn_set_chan(byte chan) {
    g_mn_cfg.chan = chan;
    mn_save_settings();
}
void mn_set_scale_type(byte scale_type) {
    g_mn_cfg.scale_type = scale_type;
    mn_save_settings();
}
void mn_set_scale_root(byte scale_root) {
    g_mn_cfg.scale_root = scale_root;
    mn_save_settings();
}
void mn_set_split_point(byte split_point) {
    g_mn_cfg.split_point = split_point;
    mn_save_settings();
}
void mn_reset_scale_root() {
    g_mn_cfg.scale_root = NO_NOTE;    
}
void mn_reset_split_point() {
    g_mn_cfg.split_point = NO_NOTE;
    g_mn_cfg.apply_above_split = 1;
}
void mn_toggle_enabled() {
    g_mn_state.enabled = !g_mn_state.enabled;
}


////////////////////////////////////////////////////////////////////////////////
// determine whether a specific note meets the filter criteria set by the 
// split point
byte mn_note_matches_split_point(byte note) {
    if(g_mn_cfg.split_point == NO_NOTE) {
        return 1;
    }
    else if(g_mn_cfg.apply_above_split) {
        return (g_mn_cfg.split_point <= note);
    }
    else {
        return (g_mn_cfg.split_point > note);        
    }
}

////////////////////////////////////////////////////////////////////////////////
// standard handling of MIDI note on/off messages - handles setting of midi channel 
// and split point, and matching of midi channel and split point. return nonzero
// if the note message should be handled
void mn_std_handle_msg(byte status, byte num_params, byte param1, byte param2) {
    if(g_mn_state.enabled) {
        byte chan = status & 0x0F;
        if(((status & 0xF0) == MIDI_STATUS_NOTE_ON) && param2) {       
            if(g_mn_cfg.chan == NO_CHAN) {
                mn_set_chan(chan);
            }
            if(g_mn_cfg.scale_root == NO_NOTE && chan == g_mn_cfg.chan) {
                mn_set_scale_root(param1);
            }
            if(g_mn_cfg.split_point == NO_NOTE && chan == g_mn_cfg.chan) {
                mn_set_split_point(param1);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// standard handling of status leds 
void mn_std_status_leds() {
    static byte ticker;
    if(!g_mn_state.enabled) {
        P_LED1 = 0;
    }
    else if(g_mn_cfg.chan == NO_CHAN || g_mn_cfg.scale_root == NO_NOTE || g_mn_cfg.split_point == NO_NOTE) {
        P_LED1 = !(ticker & 0x40);
        ++ticker;
    }
    else {
        P_LED1 = 1;
    }
}
