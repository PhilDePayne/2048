/******************************************************************************
 *
 * A03
 * 2020/21
 *
 * File:
 *    2048.h
 *
 * Description:
 *    Expose public functions related to the 2048 game.
 *
 *****************************************************************************/
#ifndef _2048_H_
#define _2048_H_

void play2048(tU8 gameType);

void startGameAsServer(void);
void startGameAsClient(void);
void displayScores(void);
void clearScores(void);

#endif
