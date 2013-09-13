/*!
\file Beacon/globals.c
\brief Functions for zeroing and calibrating sensors

\author Yuan Gao
*/

#include "all.h"

// *** LEDs and buttons stuff
volatile unsigned int flashVLED=0;		/*!< Boolean for flashing LED */
volatile unsigned int PRGTimer=0;			/*!< Timer for counting button push */
volatile unsigned char PRGLastState=0;	/*!< Boolean for last button state */
volatile unsigned int PRGPushTime=0;		/*!< Timer for counting button push */
volatile unsigned int PRGBlankTimer=0;	/*!< Timer for blanking button push (i.e. debounce) */
volatile unsigned char PRGMode=0;                 /*!< Current mode invoked by PRG mode */

// *** Timers and counters
unsigned int sysMS=0;					/*!< System milliseconds since boot */
unsigned long long sysUS=0;				/*!< System microseconds since boot */
unsigned short heartbeatWatchdog=0;		/*!< Watchdog counter to indicate when base station is lost */
unsigned short gpsWatchdog=0;			/*!< Watchdog counter to indicate when GPS is lost */