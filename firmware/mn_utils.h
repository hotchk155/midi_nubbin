enum {
    CHORD_NONE,
    CHORD_MAJ,
    CHORD_MIN,
    CHORD_DIM
};

enum {
    NOTE_C,
    NOTE_C_SHARP,
    NOTE_D,
    NOTE_D_SHARP,
    NOTE_E,
    NOTE_F,
    NOTE_F_SHARP,
    NOTE_G,
    NOTE_G_SHARP,
    NOTE_A,
    NOTE_A_SHARP,
    NOTE_B
};

typedef struct {
    byte enabled; 
    byte chan;    
    byte scale_root;
    const byte *scale;
    byte split_point;
    byte apply_above_split;
} MN_STD_STATE;

extern MN_STD_STATE g_mn;
extern const byte g_maj_scale[12];
extern const byte g_min_scale[12];

void mn_utils_reset(void);
byte mn_push_note(byte note);
byte mn_pop_note(byte note);
void mn_pop_all_notes(void);
void mn_build_triad(byte root, byte *dest);
void mn_clear_note_array(void);
void mn_add_note_to_array(byte note);
void mn_note_array_on(byte vel);
void mn_note_array_off(void);
void mn_note_array_note_on(byte index, byte vel);
void mn_note_array_track_note_msg(byte status, byte num_params, byte note, byte vel);
void mn_app_std_leds();
void mn_app_std_midi_msg(byte status, byte num_params, byte param1, byte param2);
byte mn_app_apply_to_note(byte note);

