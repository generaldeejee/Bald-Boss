/* Optional SD-card storage of 3DS arcade scores. Missing SD remains safe. */
#ifndef BALD_ELITE_3DS_SAVE_H
#define BALD_ELITE_3DS_SAVE_H
#include "game.h"
void be_save_load(BeGame *game);
void be_save_write(const BeGame *game);
#endif
