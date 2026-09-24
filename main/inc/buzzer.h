#ifndef BUZZER_H
#define BUZZER_H

// --- MUSICAL NOTES (Frequencies in Hz) ---
// The 4th Octave (Standard Middle Range)
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494

#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047  

void buzzer_init(void);
void buzzer_play_tone(int frequency, int duration_ms);
void buzzer_stop(void);#endif

#endif

