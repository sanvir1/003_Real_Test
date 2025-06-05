#include "pitches.h"

const int melody[][2] PROGMEM = {
  {NOTE_C4, 8},  // До
  {NOTE_E4, 8},  // Ми
  {NOTE_G4, 8},  // Соль
  {NOTE_C5, 8}   // До (октавой выше)
};

void playMelody() {
  for (int i = 0; i < sizeof(melody)/sizeof(melody[0]); i++) {
    int note = pgm_read_word(&melody[i][0]);
    int duration = pgm_read_word(&melody[i][1]);
    if (note == 0) {
      noTone(BUZZER_PIN);
    } else {
      tone(BUZZER_PIN, note, 1000 / duration);
    }
    delay(1000 / duration * 1.3);
  }
}