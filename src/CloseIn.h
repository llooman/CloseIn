#ifndef _CloseIn_H_
#define _CloseIn_H_

#define JL_VERSION 10402
//20000

// #define DEBUG

#define BOILER_PIN 9
#define SSR_TEMP_PIN 7
#define WATER_TEMP_PIN 6

#define NODE_ID 8

#define WATER_TEMP_ID 20
#define SSR_TEMP_ID 21

#define WATER_TEMP_MAX_ID 52
#define SSR_TEMP_MAX_ID 53

#define SSR_ID 60        // upload current dutyCycle

/*
2021-01-16 2.0.0 - refator twi 
stk500_getsync() attempt 1 of 10: not in sync: resp=0xe9   !!! push reset button release when start uploading.
2020-11-22 1.04.02 relais2 try fix overheat behaviour force evaluate always
2020-11-18 1.04.01 relais2 2.0 try fix overheat behaviour

2020-07-26 1.04.00 Upload next / if( ! espNode.isTxBufFull()
2020-06-01 10300 refactor to myTimers
                 refactor to Relais2

2020-04-12 10200
           reboot by wdt for non optiboot hangs!!
           switched to asm volatile ("  jmp 0"); 

2017-05-05 TWI_FREQ 200000L

2017-02-14 Netw2.0 update

2017-01-13 add bootCount
           retry
		   HomeNetH.h 2017-01-10  refactor bootTimer
           new EEProm class replacing EEParms and Parms
		   System::ramFree()
		   set netw.netwTimer   moved to NETW class
		   HomeNetH.h 2017-01-10  netw.sendData retry on send error
		   ArdUtils.h 2017-01-12 fix signatureChanged

2017-01-08 HomeNetH.h 2017-01-08 fix volatile

2017-01-08 add netw.execParms(cmd, id, value) in execParms
           add if(req->data.cmd != 21 || parms.parmsUpdateFlag==0 ) in loop.handle netw request
           move parms.loop(); before loop.handle netw request
           HomeNetH.h 2017-01-06

2017-01-08 TODO eeparms.exeParms  case 3: netw.netwTimer = millis() + (value * 1000); //  netwSleep

 // Boiler v 1.1  2015-12-04  events
 // Boiler v 1.01 2015-02-20  EEPROM + i2c timmings
 // Boiler v 1.0  2015-02-13
 * */

#include "Arduino.h"  //Pro Mini   optiboot   
#include <DS18B20.h>
#include "EEUtil.h"
#include <MyTimers.h>
#include <avr/wdt.h>        // watchdog
 
#include <NetwTWI.h>
#include <Relais2.h>

#define S202s02

#define TWI_PARENT
NetwTWI parentNode;

Relais2 boiler( BOILER_PIN, false, false);

#define TRACE_SEC 3

//const byte myDs[] PROGMEM =  {0x28, 0x37, 0xdb, 0x8c, 0x6, 0x0, 0x0, 0x42};  // 18

DS18B20 tempSSR( SSR_TEMP_PIN, SSR_TEMP_ID);
DS18B20 tempWater ( WATER_TEMP_PIN, WATER_TEMP_ID );
 

#define TIMERS_COUNT 4
MyTimers myTimers(TIMERS_COUNT);
#define TIMER_TRACE 0
#define TIMER_REBOOT 1
#define TIMER_TEST 2
#define TIMER_UPLOAD_ON_BOOT 3

int 	uploadOnBootCount=0;

// int heatingPerc = 0;
// int heatingPercUploaded = -1;

int evaluatePerc(bool on, bool isRunning);

void localSetVal(int id, long val);
int  upload(int id, long val, unsigned long timeStamp);
int  upload(int id, long val) ;
int  upload(int id);
int  uploadError(int id, long val);
int  handleParentReq( RxItem *rxItem) ;
int localRequest(RxItem *rxItem);
void trace( );
void reboot();

#endif