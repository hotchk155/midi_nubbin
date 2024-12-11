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

enum {
    MN_APP_CHORD,
    MN_APP_CHORD_STRUM,
    MN_APP_PITCH
};

enum {
    MN_SCALE_MAJOR,
    MN_SCALE_MINOR
};
typedef struct {
    byte chan;    
    byte scale_root;
    byte scale_type;
    byte split_point;
    byte apply_above_split;
} MN_CFG;
typedef struct {
    byte app_type;
    byte enabled; 
} MN_STATE;

extern MN_CFG g_mn_cfg;
extern MN_STATE g_mn_state;
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
void mn_app_std_leds(void);
void mn_app_std_midi_msg(byte status, byte num_params, byte param1, byte param2);
byte mn_app_apply_to_note(byte note);
inline const byte *mn_scale(byte type);
void mn_app_init();
void mn_save_settings();

///////////////////////////////////////////////////
void app_init_chord_strum(void);
void app_init_da_chord(void);