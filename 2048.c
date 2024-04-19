/******************************************************************************
 *
 * A03
 * 2020/21
 *
 * File:
 *    2048.c
 *
 * Description:
 *    Implements the 2048 game.
 *
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "../pre_emptive_os/api/osapi.h"
#include "../pre_emptive_os/api/general.h"
#include <printf_P.h>
#include <ea_init.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include "key.h"
#include "select.h"
#include "lpc2xxx.h"
#include "hw.h"
#include "uart.h"
#include "bt.h"
#include "i2c.h"
#include "eeprom.h"


/******************************************************************************
 * Typedefs and defines
 *****************************************************************************/
#define MAXROW 28
#define MAXCOL 28

#define PLAYER_CLIENT 0
#define PLAYER_HOST   1

#define GAME_TYPE_SINGLE 0
#define GAME_TYPE_DUAL_C 1
#define GAME_TYPE_DUAL_S 2

#define MAX_BT_UNITS 5
#define RECV_BUF_LEN 40

#define SCREEN_WIDTH ((tU8)130)
#define SCREEN_HEIGHT ((tU8)130)
#define CHAR_WIDTH 8

typedef struct
{
  tU8 active;
  tU8 btAddress[13];
  tU8 btName[17];
} tBtRecord;

// MUSIC TONES

#define     E5    (659.255)
#define     C5    (523.251)
#define     A5    (880.00)
#define     A6    (1760)
#define     E6    (1318.51)
#define     G4    (391.995)
#define     G5    (783.991)
#define     G6    (1567.982)
#define     B5    (987.767)
#define     H5    (932.328)
#define     C6    (1046.502)
#define     D6    (1174.659)
#define     F6    (1396.913)


/*****************************************************************************
 * Local prototypes
 ****************************************************************************/
static void setupLevel(void);
static void showGrid(tS32 tab[][4]);
static tBool isOccupied(tS32 tab[][4], tS32 y, tS32 x);
static tBool checkEnd(tS32 tab[][4]);
static void startRound(tS32 tab[][4]);
static void moveGrid(tS32 tab[][4], tS8 dir);
static void sleepLight(tU32 t);
static void playLight(tU32 hz);
static void playLED(void);
static void sleepMusic(tU32 t);
static void playNote(tU32 hz, float dlugosc);
static void playSong(void);
static void playNote(tU32, float);
static tBool checkWin(tS32 tab[][4]);
static void saveScore(tU8 score[5]);

// BLUETOOTH
static void activateServer();
static tBool checkIfClinetConnected(tU8 *pBtAddr);
static tBool searchServers(tU8 *pBtAddr);
static tBool connectToServer(tU8 *pBtAddr);
static void convertToDigits(tU8 *pBuf, tU8 value);
static tBool decodeFromDigits(tU8 *pBuf, tU8 *pValue);


/*****************************************************************************
 * Local variables
 ****************************************************************************/
// static tS8 direction = KEY_RIGHT;
static tBool done = FALSE;
static tBool connected;
static tMenu menu;
static tU8 btAddress[13];
static tU8 score;
static tU8 oppScore;
static tBtRecord foundBtUnits[MAX_BT_UNITS];
static tU8 gameType;
static tU8 recvPos;
static tU8 recvBuf[RECV_BUF_LEN];


/*****************************************************************************
 * External variables
 ****************************************************************************/
extern volatile tU32 ms;


/*****************************************************************************
 *
 * Description:
 *    Implement 2048 game
 *
 ****************************************************************************/
void play2048(tU8 gameType)
{
  tU8 keypress;
  tBool end = TRUE;
  score = 0;
  oppScore = 0;

  srand(ms);
  setupLevel();

  tS32 grid[4][4] = { {0,0,0,0},
                      {0,0,0,0},
                      {0,0,0,0},
                      {0,0,0,0}};
  startRound(grid);
  showGrid(grid);
  playLED();

  do {
    keypress = checkKey();
    if ((keypress != (tU8)KEY_NOTHING) && (keypress != (tU8)KEY_CENTER)) {
      if ((keypress == (tU8)KEY_UP) || (keypress == (tU8)KEY_RIGHT) ||
          (keypress == (tU8)KEY_DOWN) || (keypress == (tU8)KEY_LEFT)) {

        moveGrid(grid, keypress);
        startRound(grid);
        setupLevel();
        showGrid(grid);
      }
    
      if (gameType != (tU8)GAME_TYPE_SINGLE) {
        recvPos = 0;
        tU8 rxChar;

        //check if any character has been received from BT
        if (uart1GetChar(&rxChar) == (tU8)TRUE) {
          tBool done = TRUE;

          while (done == (tBool)TRUE) {
            uart1GetChar(&rxChar);
            if (rxChar == (tU8)0x0a) {
              decodeFromDigits(&recvBuf[1],  &oppScore);
              // evaluate received bytes
              if ((memcmp(recvBuf, "NO CARRIER", 10) != 0)) {
                oppScore /= 16;

                if (oppScore > score) {
                  setLED(LED_GREEN, FALSE);
                  setLED(LED_RED,   TRUE);
                } else {
                  setLED(LED_GREEN, TRUE);
                  setLED(LED_RED,   FALSE);
                }
                
                recvPos = 0;
                done = FALSE;
              } else {
                return;
              }

              recvPos = 0;
              done = FALSE;
            } else if (recvPos < (tU8)RECV_BUF_LEN) {
              recvBuf[recvPos] = rxChar;
              recvPos++;
            }
          }
        }

        //send score to bt
        tU8 buf[4];
        buf[0] = 'S';
        convertToDigits(&buf[1], (tU8) score);
        buf[3] = 0x0a;
        uart1SendChars((char *) buf, 4);
      }
    }

    if (checkEnd(grid) || checkWin(grid)) {
        end = FALSE;
        tU8 tmpScore[5];
        tmpScore[0] = '0';
        tmpScore[1] = score / (tU8)100 + (tU8)'0';
        tmpScore[2] = (score / (tU8)10) + (tU8)'0';
        tmpScore[3] = score % (tU8)10 + (tU8)'0';
        tmpScore[4] = '\0';
        saveScore(tmpScore);
        setLED(LED_GREEN, FALSE);
        setLED(LED_RED,   FALSE);
    }

  } while (end == (tBool)TRUE);

  playSong();
}


/*****************************************************************************
 *
 * Description:
 *    Initialize one level of the game. Draw game board.
 *
 ****************************************************************************/
void setupLevel()
{
  //clear screen
  lcdColor(0, 0xe0);
  lcdClrscr();

  //draw frame
  lcdGotoxy(42, 0);
  lcdPuts((const tU8 *) "2048");

  //draw game board rectangle
  lcdRect(0, 14, (4 * MAXCOL) + 4, (4 * MAXROW) + 4, 3);
  lcdRect(2, 16, 4 * MAXCOL, 4 * MAXROW, 1);
  
  lcdRect(2, 16, MAXROW, MAXROW, 196);
  lcdRect(30, 16, MAXROW, MAXROW, 196);
  lcdRect(58, 16, MAXROW, MAXROW, 196);
  lcdRect(86, 16, MAXROW, MAXROW, 196);

  lcdRect(2, 44, MAXROW, MAXROW, 196);
  lcdRect(30, 44, MAXROW, MAXROW, 196);
  lcdRect(58, 44, MAXROW, MAXROW, 196);
  lcdRect(86, 44, MAXROW, MAXROW, 196);


  lcdRect(2, 72, MAXROW, MAXROW, 196);
  lcdRect(30, 72, MAXROW, MAXROW, 196);
  lcdRect(58, 72, MAXROW, MAXROW, 196);
  lcdRect(86, 72, MAXROW, MAXROW, 196);


  lcdRect(2, 100, MAXROW, MAXROW, 196);
  lcdRect(30, 100, MAXROW, MAXROW, 196);
  lcdRect(58, 100, MAXROW, MAXROW, 196);
  lcdRect(86, 100, MAXROW, MAXROW, 196);
}

/*****************************************************************************
 *
 * Description:
 *    Display values of all cells in the game grid.
 *
 ****************************************************************************/

void showGrid(tS32 tab[][4])
{
  tS32 i;
  tS32 k;
  for (i = 0; i < 4; ++i) {
    for (k = 0; k < 4; ++k) {
      lcdGotoxy((k*MAXCOL)+20,(i*MAXROW)+20);
      switch (tab[i][k]) 
      {
      case 0:
        lcdPuts((const tU8 *) "0");
          break;
      case 2:
          lcdPuts((const tU8 *) "2");
          break;
      case 4:
          lcdPuts((const tU8 *) "4");
          break;
      case 8:
          lcdPuts((const tU8 *) "8");
          break;
      case 16:
          lcdPuts((const tU8 *) "16");
          break;
      case 32:
          lcdPuts((const tU8 *) "32");
          break;
      case 64:
          lcdPuts((const tU8 *) "64");
          break;
      case 128:
          lcdPuts((const tU8 *) "128");
          break;
      case 256:
          lcdPuts((const tU8 *) "256");
          break;
      case 512:
          lcdPuts((const tU8 *) "512");
          break;
      case 1024:
          lcdPuts((const tU8 *) "1024");
          break;
      case 2048:
          lcdPuts((const tU8 *) "2048");
          break;
      default:
          break;
      }
    }
  }
}

/*****************************************************************************
 *
 * Description:
 *    Check whether cell is occupied (cell value greater than zero).
 * 
 * Return: TRUE if cell is occupied
 *         FALSE if cell is empty
 *
 ****************************************************************************/

tBool isOccupied(tS32 tab[4][4], tS32 y, tS32 x)
{
  return (tab[y][x]);
}

/*****************************************************************************
 *
 * Description:
 *    Check if game ended (no empty cells).
 * 
 * Return: TRUE if no empty cells
 *         FALSE if there are empty cells
 *
 ****************************************************************************/

tBool checkEnd(tS32 tab[][4])
{
  tS32 i, k;
  for (i = 0; i < 4; ++i) {
    for (k = 0; k < 4; ++k) {
      if (tab[i][k] == 0) {
        return FALSE;
      }
    }
  }
  return TRUE;
}

/*****************************************************************************
 *
 * Description:
 *    Check if game ended (a cell has a value of 2048).
 * 
 * Return: TRUE if a cell has a value of 2048
 *         FALSE if no cell has a value of 2048
 *
 ****************************************************************************/

tBool checkWin(tS32 tab[][4])
{ 
  tS32 i,k;
  for (i = 0; i < 4; ++i) {
    for (k = 0; k < 4; ++k) {
      if (tab[i][k] == 2048) {
          return TRUE;
      }
    }
  }
  return FALSE;
}

/*****************************************************************************
 *
 * Description:
 *    Selects 2 random empty cells (1 if only one left) and sets their values
 *    to 2.
 *
 ****************************************************************************/

void startRound(tS32 tab[][4])
{
  tBool set;
  tS32 i;
  for (i = 0; i < 2; ++i) {
    set = FALSE;
    while (!set) {
      if (checkEnd(tab) == (tBool)TRUE) {
        set = TRUE;
      }
      tS32 y = rand() % 4;
      tS32 x = rand() % 4;
      if (!isOccupied(tab, y, x)) {
          tab[y][x] = 2;
          set = TRUE;
      }
    }
  }
}

/*****************************************************************************
 *
 * Description:
 *    Moves the cells in the grid according to the received input.
 *    Connects two cells of the same value if they collide.
 *    Increases the score by one each time two cells are connected.
 *
 ****************************************************************************/

void moveGrid(tS32 tab[4][4], tS8 direction)
{
  tS32 i,k;
  switch (direction) {
    case KEY_UP:
      for (i = 3; i > 0; --i) {
        for (k = 3; k >= 0; --k) {
          if (isOccupied(tab, i, k) != (tBool)0) {
            if (isOccupied(tab, i - 1, k) != (tBool)0) {
              if (tab[i][k] == tab[i - 1][k]) {
                score += (tU8)1;
                tab[i][k] = 0;
                tab[i - 1][k] *= 2;
              }
              continue;
            }
            tab[i - 1][k] = tab[i][k];
            tab[i][k] = 0;
          }
        }
      }
      break;

    case KEY_DOWN:
      for ( i = 0; i < 4; ++i) {
        for (k = 0; k < 4; ++k) {
          if (isOccupied(tab, i, k) != (tBool)0) {
            if (isOccupied(tab, i + 1, k) != (tBool)0) {
              if (tab[i][k] == tab[i + 1][k]) {
                score += (tU8)1;
                tab[i][k] = 0;
                tab[i + 1][k] *= 2;
              }
              continue; 
            }
            tab[i + 1][k] = tab[i][k];
            tab[i][k] = 0;
          }
        }
      }
      break;

    case KEY_LEFT:
      for (i = 0; i < 4; ++i) {
        for (k = 3; k > 0; k--) {
          if (isOccupied(tab, i, k) != (tBool)0) {
            if (isOccupied(tab, i, k - 1) != (tBool)0) {
              if (tab[i][k] == tab[i][k - 1]) {
                score += (tU8)1;
                tab[i][k] = 0;
                tab[i][k - 1] *= 2;
              }
              continue;
            }
            tab[i][k - 1] = tab[i][k];
            tab[i][k] = 0;
          }
        }
      }
      break;

    case KEY_RIGHT:
      for (i = 0; i < 4; ++i) {
        for (k = 0; k < 3; ++k) {
          if (isOccupied(tab, i, k) != (tBool)0) {
            if (isOccupied(tab, i, k + 1) != (tBool)0) {
              if (tab[i][k] == tab[i][k + 1]) {
                score += (tU8)1;
                tab[i][k] = 0;
                tab[i][k + 1] *= 2;
              }
              continue;
            }
            tab[i][k + 1] = tab[i][k];
            tab[i][k] = 0;
          }
        }
      }
      break;
      
      default:
          break;
  }
}

/*****************************************************************************
 *
 * Description:
 *    Sets the pause between the switching of LED.
 *
 ****************************************************************************/

void sleepLight(tU32 t)
{
  PWM_TCR = 0x02;
  PWM_PR = 0x00; 
  PWM_MR0 = (FOSC * 1 / 5000000) * t;
  PWM_IR = 0xff; 
  PWM_MCR = 0x04;
  PWM_TCR = 0x01;
  while ((PWM_TCR & 0x01) != 0) {};
}

/*****************************************************************************
 *
 * Description:
 *    Turns on LED with a specified intensity.
 *
 ****************************************************************************/

void playLight(tU32 hz)
{
  tU32 i;
  for (i = 0; i < (tU32)1000; ++i) {
    setLED(LED_GREEN, TRUE);
    sleepLight(hz / (tU32)100);
    setLED(LED_GREEN, FALSE);
    sleepLight((tU32)9600000 / hz);
  }
  sleepLight(200000);
}

/*****************************************************************************
 *
 * Description:
 *    Turns on the LED with different intensities.
 *
 ****************************************************************************/

void playLED(void)
{
  playLight(800000);
  playLight(300000);
  playLight(200000);
  playLight(100000);
  playLight(80000);
  playLight(40000);
  playLight(20000);
  playLight(14000);
  playLight(10000);
  playLight(8000);
  playLight(2500);
  playLight(1000);
}

/*****************************************************************************
 *
 * Description:
 *    Sets the pause between the switching of the buzzer.
 *
 ****************************************************************************/

void sleepMusic(tU32 t)
{
  TIMER1_TCR = 0x02;
  TIMER1_PR = 0x00; 
  TIMER1_MR0 = (FOSC * 1 / 5000000) * t;
  TIMER1_IR = 0xff; 
  TIMER1_MCR = 0x04;
  TIMER1_TCR = 0x01;
  while ((TIMER1_TCR & 0x01) != 0) {};
}

/*****************************************************************************
 *
 * Description:
 *    Plays a note.
 *
 ****************************************************************************/

void playNote(tU32 hz, float dlugosc)
{
  tU32 i;
  for (i = 0; i < (hz * (tU32)dlugosc); ++i) 
  {
    setBuzzer(TRUE);
    sleepMusic((tU32)800000/hz); 
    setBuzzer(FALSE);
    sleepMusic((tU32)9600000/hz); 
  }
  sleepMusic(200000);
}

/*****************************************************************************
 *
 * Description:
 *    Plays a song (a sequence of notes).
 *
 ****************************************************************************/

void playSong(void)
{
  tS32 i;

  //Mario

  playNote(E5,0.125*1.1);
  playNote(E5,0.125*1);
  sleepMusic(500000);
  playNote(E5,0.125*1.3);
  sleepMusic(600000);
  playNote(C5,0.125*1);
  playNote(E5,0.125*1.1);
  sleepMusic(600000);
  playNote(G5,0.125*1.3);
  sleepMusic(3000000);
  playNote(G4,0.125*2.3);

  for (i = 0; i < 2; ++i) {
    sleepMusic(2000000);
    playNote(C6,0.125*1.5);
    sleepMusic(1500000);
    playNote(G5,0.125*1.5);
    sleepMusic(1500000);
    playNote(E5,0.125*1.5);
    sleepMusic(1500000);
    playNote(A5,0.125*1.1);
    sleepMusic(600000);
    playNote(B5,0.125*1.1);
    sleepMusic(600000);
    playNote(H5,0.125*1);
    sleepMusic(500000);
    playNote(A5,0.125*1.3);
    sleepMusic(1000000);
    playNote(G5,0.125*1.3);
    sleepMusic(800000);
    playNote(E6,0.125*1.1);
    sleepMusic(400000);
    playNote(G6,0.125*1.3);
    sleepMusic(400000);
    playNote(A6,0.125*1.6);
    sleepMusic(600000);
    playNote(F6,0.125*1.1);
    sleepMusic(400000);
    playNote(G6,0.125*1.6);
    sleepMusic(400000);
    playNote(E6,0.125*1.6);
    sleepMusic(400000);
    playNote(C6,0.125*1.3);
    sleepMusic(600000);
    playNote(D6,0.125*1.1);
    sleepMusic(400000);
    playNote(B5,0.125*1.6);
    }
}

/******************************************************************************
 ******************************************************************************
 * BLUETOOTH HANDLING PARTS
 ******************************************************************************
 *****************************************************************************/
#define RECV_BUF_LEN 40
static tU8 recvPos;
static tU8 recvBuf[RECV_BUF_LEN];


/******************************************************************************
 * Activate server functionality
 *****************************************************************************/
static void
activateServer()
{
  osSleep(100);
  uart1SendString((const tU8 *) "+++");
  osSleep(100);
  uart1SendString((const tU8 *) "AT+BTCAN\r");
  osSleep(50);
  uart1SendString((const tU8 *) "AT+BTSRV=20,\"2048Server\"\r");
  recvPos = 0;
}

/******************************************************************************
 * Return: TRUE if connection established
 *         FALSE if no connection established
 *****************************************************************************/
static tBool
checkIfClinetConnected(tU8 *pBtAddr)
{
  tU8 rxChar;

  //check if any character has been received from BT
  if (uart1GetChar(&rxChar) == (tU8)TRUE)
  {
    printf("%c", rxChar);

    if (rxChar == (tU8)0x0a)
    {
      if (recvPos > (tU8)0){
        recvBuf[recvPos-(tU8)1] = '\0';
      }

      //evaluate received bytes
      if ((memcmp(recvBuf, "CONNECT ", 8) == 0) && (recvPos == (tU8)21))
      {
        for (recvPos=(tU8)0; recvPos<(tU8)12; recvPos++)
        {
          *pBtAddr = recvBuf[recvPos + (tU8)8];
          pBtAddr++;
        }
        *pBtAddr = '\0';

        return TRUE;
      }
      recvPos = 0;
    }
    else if (recvPos < (tU8)RECV_BUF_LEN) {
      recvBuf[recvPos] = rxChar;
      recvPos++;
    }
  }

  //no connection established
  return FALSE;
}


/******************************************************************************
 * Return: TRUE if connection succeeded
 *         FALSE if connection failed
 *****************************************************************************/
static tBool
connectToServer(tU8 *pBtAddr)
{
  volatile tU32 timeStamp;
  tU8 connected;
  tU8 rxChar;
  tU32 cnt = 0;

  osSleep(100);
  uart1SendString((const tU8 *) "+++");
  osSleep(100);
  uart1SendString((const tU8 *) "AT+BTCAN\r");
  osSleep(50);
  uart1SendString((const tU8 *) "AT+BTCLT=\"");
  uart1SendString((const tU8 *)pBtAddr);
  uart1SendString((const tU8 *) "\",20,3\r");
  osSleep(100);

  //wait for response "CONNECT <BTADDR>" 
  timeStamp = ms;
  connected = FALSE;
  recvPos = 0;
  while ((connected == (tU8)FALSE) && ((ms - timeStamp) < (tU32)10000))
  {
    //check if any character has been received from BT
    if (uart1GetChar(&rxChar) == (tU8)TRUE)
    {
      printf("%c", rxChar);

      if (rxChar == (tU8)0x0a)
      {
        if (recvPos > (tU8)0) {
          recvBuf[recvPos-(tU8)1] = '\0';
        }

        //evaluate received bytes
        if ((memcmp(recvBuf, "CONNECT ", 8) == 0) && (recvPos == (tU8)21))
        {
          connected = TRUE;
        }
        else if ((memcmp(recvBuf, "NO CARRIER", 10) == 0))
        {
          return FALSE;
        }
        recvPos = 0;
      }
      else if (recvPos < (tU8)RECV_BUF_LEN) {
        recvBuf[recvPos] = rxChar;
        recvPos++;
      }
    }
    osSleep(1);

    //prtS32 activity indicator
    lcdGotoxy(88,18);
    lcdColor(0x00,0xfd);
    switch (cnt % (tU32)150)
    {
    case   0: lcdPuts((const tU8 *) "    ");break;
    case  25: lcdPuts((const tU8 *) ".   ");break;
    case  50: lcdPuts((const tU8 *) "..  ");break;
    case  75: lcdPuts((const tU8 *) "... ");break;
    case 100: lcdPuts((const tU8 *) " .. ");break;
    case 125: lcdPuts((const tU8 *) "  . ");break;
    default: break;
    }
    cnt++;
  }

  //wait for accpet from server
  if (connected == (tU8)TRUE)
  {
    //wait for response "LETS START PLAYING" 
    timeStamp = ms;
    connected = FALSE;
    recvPos = 0;
    while ((connected == (tU8)FALSE) && ((ms - timeStamp) < (tU32)10000))
    {
      //check if any character has been received from BT
      if (uart1GetChar(&rxChar) == (tU8)TRUE)
      {
        printf("%c", rxChar);

        if (rxChar == (tU8)0x0a)
        {
          if (recvPos > (tU8)0) {
            recvBuf[recvPos-(tU8)1] = '\0';
        }
          //evaluate received bytes
          if (memcmp(recvBuf, "LETS START PLAYING", 18) == 0)
          {
            return TRUE;
          }
          else if ((memcmp(recvBuf, "NO CARRIER", 10) == 0))
          {
            return FALSE;
          }
          recvPos = 0;
        }
        else if (recvPos < (tU8)RECV_BUF_LEN) {
          recvBuf[recvPos] = rxChar;
          recvPos++;
        }
      }
      osSleep(1);

      //prtS32 activity indicator
      lcdGotoxy(88,18);
      lcdColor(0x00,0xfd);
      switch (cnt % (tU32)150)
      {
      case   0: lcdPuts((const tU8 *) "   ");break;
      case  25: lcdPuts((const tU8 *) "*  ");break;
      case  50: lcdPuts((const tU8 *) "** ");break;
      case  75: lcdPuts((const tU8 *) "*** ");break;
      case 100: lcdPuts((const tU8 *) " ** ");break;
      case 125: lcdPuts((const tU8 *) "  * ");break;
      default: break;
      }
      cnt++;
    }
  }

  return FALSE;
}


/*****************************************************************************
 *
 * Description:
 *    Prints the list of 'found' Bluetooth units
 *
 ****************************************************************************/
static void
drawBtsFound(tU8 cursorPos)
{
  tU32 row;

  for(row=0; row<(tU32)MAX_BT_UNITS; row++)
  {
    lcdGotoxy((tU8)2,(tU8)30+((tU8)14*row));
    if (foundBtUnits[row].active == (tU8)FALSE)
    {
      if (cursorPos == row) {
        lcdColor(0x01,0xe0);
      } else {
        lcdColor(0x00,0xfd);
      }
      lcdPuts((const tU8 *) "-");
    }
    else
    {
      if (cursorPos == row) {
        lcdColor(0x01,0xe0);
      } else {
        lcdColor(0x00,0xfd);
      }
      lcdPuts((const tU8 *)foundBtUnits[row].btAddress);
    }
  }
}


/******************************************************************************
 * Perform a BT inquiry after 2048 servers
 *****************************************************************************/
static tBool
searchServers(tU8 *pBtAddr)
{
  tU8  rxChar;
  tU8  done;
  tU8  i;
  tU8  anyKey;
  tU8  cursorPos;
  volatile tU32 timeStamp;
  tU8  recvPos;
  tU8  recvBuf[RECV_BUF_LEN];
  tU8  foundBt;
  tU32 cnt;

  for(foundBt=0; foundBt<(tU8)MAX_BT_UNITS; foundBt++)
  {
    foundBtUnits[foundBt].active = FALSE;
    foundBtUnits[foundBt].btName[0] = '\0';
  }

  //clear menu screen
  lcdRect(1, 16, 126, 84, 0x00);
  lcdGotoxy(18,16);
  lcdColor(0x00,0xfd);
  lcdPuts((const tU8 *) "Inquiry");

  //*************************************************************
  //* Start inquiry (during 6 seconds)
  //*************************************************************
  osSleep(100);
  uart1SendString((const tU8 *) "+++");
  osSleep(100);
  uart1SendString((const tU8 *) "AT+BTCAN\r");
  osSleep(50);
  uart1SendString((const tU8 *) "AT+BTINQ=6\r");

  //*************************************************************
  //* Get (receive and interpret) discovered units
  //*************************************************************
  timeStamp = ms;
  done = FALSE;
  recvPos = 0;
  foundBt = 0;
  cnt = 0;
  while ((done == (tU8)FALSE) && ((ms - timeStamp) < (tU32)6500))
  {
    //check if any character has been received from BT
    if (uart1GetChar(&rxChar) == (tU8)TRUE)
    {
      printf("%c", rxChar);
      
      if (rxChar == (tU8)0x0a)
      {
        if (recvPos > (tU8)0) {
          recvBuf[recvPos-(tU8)1] = '\0';
        }
        //evaluate received bytes
        if ((memcmp(recvBuf, "+BTINQ: ", 8) == 0) && (recvPos == (tU8)29))
        {
          if (foundBt < (tU8)MAX_BT_UNITS)
          {
            for(recvPos=0; recvPos<(tU8)12; recvPos++) {
              foundBtUnits[foundBt].btAddress[recvPos] = recvBuf[recvPos + (tU8)8];
            }
            foundBtUnits[foundBt].btAddress[12] = '\0';
//            foundBtUnits[foundBt].active = TRUE;
            foundBt++;

          }
        }
        else if (memcmp(recvBuf, "+BTINQ: COMPLETE", 16) == 0)
        {
          done = TRUE;
        }
        recvPos = 0;
      }
      else if (recvPos <(tU8) RECV_BUF_LEN) {
        recvBuf[recvPos] = rxChar;
        recvPos++;
      }
    }
    osSleep(1);

    //prtS32 activity indicator
    lcdGotoxy(74,16);
    lcdColor(0x00,0xfd);
    switch(cnt % (tU32)150)
    {
      case   0: lcdPuts((const tU8 *) "   ");break;
      case  25: lcdPuts((const tU8 *) ".  ");break;
      case  50: lcdPuts((const tU8 *) ".. ");break;
      case  75: lcdPuts((const tU8 *) "...");break;
      case 100: lcdPuts((const tU8 *) " ..");break;
      case 125: lcdPuts((const tU8 *) "  .");break;
      default: break;
    }
    cnt++;
  }

  //*************************************************************
  //* Get name of discovered bt units
  //*************************************************************
  for(i=0; i<foundBt; ++i)
  {
    uart1SendString((const tU8 *) "AT+BTSDP=");
    uart1SendString((const tU8 *)foundBtUnits[i].btAddress);
    uart1SendString((const tU8 *) "\r");

    timeStamp = ms;
    done = FALSE;
    recvPos = 0;

    while ((done == (tU8)FALSE) && ((ms - timeStamp) < (tU32)100000))
    {
      //check if any character has been received from BT
      if (uart1GetChar(&rxChar) == (tU8)TRUE)
      {
        printf("%c", rxChar);
      
        if (rxChar == (tU8)0x0a)
        {
          if (recvPos > (tU8)0) {
            recvBuf[recvPos-(tU8)1] = '\0';
        }
          //evaluate received bytes
          if (memcmp(recvBuf, "+BTSDP: COMPLETE", 16) == 0) {
            done = TRUE;
          }
          else if (memcmp(recvBuf, "+BTSDP: ", 8) == 0)
          {
            tU8 *pStr;
            
            //seach for service name
            pStr = (tU8*) strchr((char *) &recvBuf[10], '\"');
            if (pStr != ( void * )NULL)
            {
              pStr++;

              //get string (to '"')
              if (0 == strncmp((const char *)"2048Server", (const char *) pStr, 14)) {
                foundBtUnits[i].active = TRUE;
              }
            }
          }
          recvPos = 0;
        }
        else if (recvPos < (tU8)RECV_BUF_LEN) {
          recvBuf[recvPos] = rxChar;
          recvPos++;
        }
      }
      osSleep(1);
      //prtS32 activity indicator
      lcdGotoxy(74,16);
      lcdColor(0x00,0xfd);
      switch(cnt % (tU32)150)
      {
        case   0: lcdPuts((const tU8 *) "   ");break;
        case  25: lcdPuts((const tU8 *) "*  ");break;
        case  50: lcdPuts((const tU8 *) "** ");break;
        case  75: lcdPuts((const tU8 *) "***");break;
        case 100: lcdPuts((const tU8 *) " **");break;
        case 125: lcdPuts((const tU8 *) "  *");break;
        default: break;
      }
      cnt++;
    }
  }

  lcdGotoxy(74,16);
  lcdColor(0x00,0xfd);
  lcdPuts((const tU8 *) "-done");

  //*************************************************************
  //* Handle user key inputs (move between discovered units)
  //*************************************************************
  done = FALSE;
  cursorPos = 0;
  drawBtsFound(cursorPos);
  while (done == (tU8)FALSE)
  {
    anyKey = checkKey();
    if (anyKey != (tU8)KEY_NOTHING)
    {
      //exit and save new name
      if (anyKey == (tU8)KEY_CENTER)
      {
        done = TRUE;
      }

      //
      else if (anyKey == (tU8)KEY_UP)
      {
        if (cursorPos > (tU8)0) {
          cursorPos--;
        } else {
          cursorPos = MAX_BT_UNITS-1;
        }
        drawBtsFound(cursorPos);
      }
      
      //
      else if (anyKey == (tU8)KEY_DOWN)
      {
        if (cursorPos < (tU8)MAX_BT_UNITS-(tU8)1) {
          cursorPos++;
        } else {
          cursorPos = 0;
        }
        drawBtsFound(cursorPos);
      }
    }
    osSleep(1);
  }
  
  if (foundBtUnits[cursorPos].active == (tU8)TRUE)
  {
    strcpy((char *) pBtAddr, (char *) foundBtUnits[cursorPos].btAddress);
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/*****************************************************************************
 *
 * Description:
 *    Display string one character at a time
 *
 ****************************************************************************/
static void
displayMessage(const tU8 *pMessage, tU8 speed)
{
  while (*pMessage != '\0')
  {
    lcdPutchar(*pMessage++);
    osSleep(speed);
  }
}

/*****************************************************************************
 *
 * Description:
 *    Starts a 2048 server for multiplayer mode.
 *
 ****************************************************************************/

void startGameAsServer(void)
{
    //block BT process to communicate with BGB203
  blockBtProc();
    tU32 cnt = 0;
      tU8  key;

      //prtS32 "waiting for connection" message...
      lcdRect(2, 16, 125, 84, 0x6d);
      lcdRect(4, 18, 121, 80, 0x00);
      lcdColor(0x00,0xfd);
      lcdGotoxy(8,18);
      lcdPuts((const tU8 *) "Waiting for");
      lcdGotoxy(8,32);
      lcdPuts((const tU8 *) "client conn");

      gameType = GAME_TYPE_DUAL_S;
      activateServer();

      connected = FALSE;
      while (connected == (tBool)FALSE)
      {
        key = checkKey();

        if ((tBool)TRUE == checkIfClinetConnected(btAddress))
        {
          //prtS32 from which BT address
          lcdGotoxy(8,18);
          lcdPuts((const tU8 *) "Request from");
          lcdGotoxy(8,32);
          lcdPuts((const tU8 *)btAddress);
          lcdPuts((const tU8 *) "  ");

          //ask player if start playing with connected client
          menu.xPos = 10;
          menu.yPos = 50;
          menu.xLen = 6+(13*8);
          menu.yLen = 5*14;
          menu.noOfChoices = 2;
          menu.initialChoice = 0;
          menu.pHeaderText = (const tU8 *) "Accept conn? ";
          menu.headerTextXpos = 17;
          menu.pChoice[0] = (const tU8 *) "Start playing ";
          menu.pChoice[1] = (const tU8 *) "Refuse ";
          menu.bgColor       = 0;
          menu.borderColor   = 0x6d;
          menu.headerColor   = 0;
          menu.choicesColor  = 0xfd;
          menu.selectedColor = 0xe0;

          switch (drawMenu(menu))
          {
          case 0: uart1SendString((const tU8 *) "\nLETS START PLAYING\n"); done = FALSE; break;  //start playing as server
          case 1: uart1SendString((const tU8 *) "+++"); done = TRUE; break;                      //refuse connection attempt and cancel game
          default: break;
          }
          connected = TRUE;
        }

        //check if any key pressed to cancel waiting
        else if (key != (tU8)KEY_NOTHING)
        {
          //cancel server and exit game
          done = TRUE;
          break;
        }

        else
        {
          //prtS32 activity indicator
          lcdGotoxy(96,30);
          lcdColor(0x00,0xfd);
          switch (cnt % (tU32)150)
          {
          case   0: lcdPuts((const tU8 *) "   ");break;
          case  25: lcdPuts((const tU8 *) ".  ");break;
          case  50: lcdPuts((const tU8 *) ".. ");break;
          case  75: lcdPuts((const tU8 *) "...");break;
          case 100: lcdPuts((const tU8 *) " ..");break;
          case 125: lcdPuts((const tU8 *) "  .");break;
          default: break;
          }
          cnt++;
          osSleep(1);
        }
      }
      play2048(gameType);
}

/*****************************************************************************
 *
 * Description:
 *    Connects to a 2048 server for multiplayer mode.
 *
 ****************************************************************************/

void startGameAsClient(void)
{
    //block BT process to communicate with BGB203
  blockBtProc();
    //Dual player client
    gameType = GAME_TYPE_DUAL_C;

    lcdRect(2, 16, 125, 84, 0x6d);
    lcdRect(4, 18, 121, 80, 0x00);
    lcdColor(0x00,0xfd);

    //search for available servers...
    if ((tBool)TRUE == searchServers(btAddress))
    {
      //prtS32 "trying to connect" message...
      lcdRect(2, 16, 125, 84, 0x6d);
      lcdRect(4, 18, 121, 80, 0x00);
      lcdColor(0x00,0xfd);
      lcdGotoxy(8,18);
      lcdPuts((const tU8 *) "Connecting");

      //connect
      if ((tBool)TRUE == connectToServer(btAddress)) {
        done = FALSE; //start playing as client
      }
      else
      {
        lcdGotoxy(8,32);
        displayMessage((const tU8 *) "Failed to", 3);
        lcdGotoxy(8,46);
        displayMessage((const tU8 *) "connect...", 3);
        osSleep(150);

        //failed to connect
        done = TRUE;
      }
    }
    else {
      done = TRUE;
    }
    if (done == (tBool)TRUE) {
    play2048(gameType);
    }
}

/*****************************************************************************
 *
 * Description:
 *    Saves current score to EEPROM.
 *
 ****************************************************************************/

void saveScore(tU8 actualScore[])
{
  tU8 readedScore[8][4];

  tS32 i, k;
  for (i = 0; i < 8; ++i) {
    eepromPageRead(i * 4, readedScore[i], 4);
    for (k = 0; k < 4; ++k) {
      if (readedScore[i][k] < actualScore[k]) {
        eepromWrite(i * 4, actualScore, 4);
        eepromPoll();
        return;
      }
    }
  }
}

/*****************************************************************************
 *
 * Description:
 *    Clears scores written in EEPROM.
 *
 ****************************************************************************/

void clearScores()
{
  tU8 clear[5] = {0, 0 , 0 , 0, 0};

  tS32 i;
  for (i = 0; i < 8; ++i) {
    eepromWrite(i * 4, clear, 4);
    eepromPoll();
  }
}

/*****************************************************************************
 *
 * Description:
 *    Displays 8 best scores.
 *
 ****************************************************************************/

void displayScores()
{
  tU8 readedScore[8][5];

  tS32 i;
  for (i = 0; i < 8; ++i) {
    readedScore[i][4] = 0;
    eepromPageRead(i * 4, readedScore[i], 4);
  }

  lcdClrscr();
  lcdColor(0x6d,0);
  lcdRectBrd(0, 1, 128, 128, 0, 25, 0x6d);

  tS32 j;
  for (j = 0; j < 8; ++j) {
    lcdGotoxy(30, (j * 16) + 4);
    lcdPuts((const tU8 *)readedScore[j]);
  }

  while (checkKey() != KEY_CENTER) {};
}

/*****************************************************************************
 *
 * Description:
 *    Converts value to hex digit for UART operations.
 *
 ****************************************************************************/

static void convertToDigits(tU8 *pBuf, tU8 value)
{
  static const tU8 toHex[16] = {'0', '1', '2', '3',
                                '4', '5', '6', '7',
                                '8', '9', 'A', 'B',
                                'C', 'D', 'E', 'F'};

  *pBuf = toHex[value >> 4];
  *pBuf += 1;
  *pBuf = toHex[value & (tU8)0x0f];
}

/*****************************************************************************
 *
 * Description:
 *    Converts hex digits to dec for UART operations.
 *
 ****************************************************************************/

static tBool decodeFromDigits(tU8 *pBuf, tU8 *pValue)
{
  tU8 result = 99;
  tBool valid = TRUE;
  
  if ((*pBuf >= '0') && (*pBuf <= '9')) {
    result = *pBuf - 48; // 48 = '0' w ASCII
  } else if ((*pBuf >= 'A') && (*pBuf <= 'F')) {
    result = *pBuf - 65 + 10; // 65 = 'A' w ASCII
  } else {
    valid = FALSE;
  }

  result <<= 4;
  pBuf++;

  if ((*pBuf >= '0') && (*pBuf <= '9')) {
    result |= *pBuf - 48; // 48 = '0' w ASCII
  } else if ((*pBuf >= 'A') && (*pBuf <= 'F')) {
    result |= *pBuf - 65 + 10; // 65 = 'A' w ASCII
  } else {
    valid = FALSE;
  }

  *pValue = result;
  return valid;
}
