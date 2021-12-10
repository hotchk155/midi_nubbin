#include <system.h>
#include <memory.h>
#include "mw_common.h"
// configuration words: 16MHz internal oscillator block, reset disabled
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF & _CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000
#define FIRMWARE_VERSION 1



#define K_INC		K_KEY_A
#define K_DEC		K_KEY_B
#define K_INC_OCT	K_KEY_C
#define K_DEC_OCT	K_KEY_D
#define K_RESET		K_KEY_E

#define MAX_SEMI	6
#define MIN_SEMI	-6

#define MAX_OCTAVE	4
#define MIN_OCTAVE	-4

int transpose_semitone = 0;
int transpose_octave = 0;

void on_key_down(byte key) {
	switch(key) {
		case K_INC:
			if(transpose_semitone < MAX_SEMI) {
				++transpose_semitone;
			}
			break;
		case K_DEC:
			if(transpose_semitone > MIN_SEMI) {
				--transpose_semitone;
			}
			break;
		case K_INC_OCT:
			if(transpose_octave < MAX_OCTAVE) {
				++transpose_octave;
			}
			break;
		case K_DEC_OCT:
			if(transpose_octave > MIN_OCTAVE) {
				--transpose_octave;
			}
			break;
		case K_RESET:
	}
}
void on_key_up(byte key) {
}
byte on_clock(byte ch) {
	return 0;
}


void on_note(byte status, byte *note) {
	*note += transpose;

}


////////////////////////////////////////////////////////////
// ENTRY POINT
void main()
{ 
	mw_init();
	for(;;) {
		mw_run();
	}
}
