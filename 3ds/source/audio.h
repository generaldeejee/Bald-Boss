/* Optional procedural 3DS audio: gameplay still works without DSP firmware. */
#ifndef BALD_ELITE_3DS_AUDIO_H
#define BALD_ELITE_3DS_AUDIO_H
#include <stdbool.h>
bool be_audio_init(void);
void be_audio_update(bool music,bool shaving);
void be_audio_victory(void);
void be_audio_shutdown(void);
#endif
