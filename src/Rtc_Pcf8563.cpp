/****c*
 *  NAME
 *    Pcf8563 Real Time Clock support routines
 *  AUTHOR
 *    Joe Robertson, jmr
 *    orbitalair@gmail.com
 *    
 *  CREATION DATE
 *    9/24/06,  init - built off of usart demo.  using mikroC
 *  NOTES
 *  HISTORY
 *    10/14/06 ported to CCS compiler, jmr
 *    2/21/09  changed all return values to hex val and not bcd, jmr
 *    1/10/10  ported to arduino, jmr
 *    2/14/10  added 3 world date formats, jmr
 *    1/22/10, jmr,  ported to arduino 1.0.  changed header include, wire.send -> wire.write, wire.receive -> wire.read
 *    28/02/2012 A. Pasotti
 *             fixed a bug in RTCC_ALARM_AF,
 *             added a few (not really useful) methods
 *    12/26/12, jmr, merged in fix for arduino 1.0.1 Wire.
 *                   added version method to print out code version.
 *    2/6/15  made gettime, getdate more convenient
 *                   rechecked and added to examples.  ino's are most up to date.
 *  TODO
 *    done- Add Euro date format
 *    done- Add short time (hh:mm) format
 *    Add 24h/12h format
 *    Add timer support
 ******
 *  Robodoc embedded documentation.
 *  http://www.xs4all.nl/~rfsber/Robo/robodoc.html
 */
/*  Copyright (c) 2006-2020 Joe Robertson. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * or https://www.gnu.org/licenses/lgpl-3.0.html
 */
 
 
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#define byte uint8_t
#define send( X ) write( static_cast<uint8_t>(X) )
#define receive( X ) read( X )
#else
#include "WProgram.h"
#endif


#include "Rtc_Pcf8563.h"

Rtc_Pcf8563::Rtc_Pcf8563(WireBase& wire) :WW(wire)
    {
    WW.begin();
    Rtcc_Addr = RTCC_R >> 1;
    }

Rtc_Pcf8563::Rtc_Pcf8563() :Rtc_Pcf8563(Wire) {};

void Rtc_Pcf8563::initClock()
{     
  WW.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
  WW.send(0x0);       // start address
    
  WW.send(0x0);   //control/status1
  WW.send(0x0);   //control/status2
  WW.send(0x01);  //set seconds
  WW.send(0x01);  //set minutes
  WW.send(0x01);  //set hour
  WW.send(0x01);  //set day
  WW.send(0x01);  //set weekday
  WW.send(0x01);  //set month, century to 1
  WW.send(0x01);  //set year to 99
  WW.send(0x80);  //minute alarm value reset to 00
  WW.send(0x80);  //hour alarm value reset to 00
  WW.send(0x80);  //day alarm value reset to 00
  WW.send(0x80);  //weekday alarm value reset to 00
  WW.send(0x0);   //set SQW, see: setSquareWave
  WW.send(0x0);   //timer off
  WW.endTransmission();
  
}

/* Private internal functions, but useful to look at if you need a similar func. */
byte Rtc_Pcf8563::decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

byte Rtc_Pcf8563::bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}


void Rtc_Pcf8563::clearStatus()
{
  WW.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
    WW.send(0x0);
  WW.send(0x0);               //control/status1
  WW.send(0x0);               //control/status2
  WW.endTransmission();  
}

void Rtc_Pcf8563::setTime(byte hour, byte minute, byte sec)
{
  WW.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
  WW.send(RTCC_SEC_ADDR);     // send addr low byte, req'd

  WW.send(decToBcd(sec));         //set seconds
  WW.send(decToBcd(minute));  //set minutes
  WW.send(decToBcd(hour));        //set hour
    WW.endTransmission();
   
}

void Rtc_Pcf8563::setDate(byte day, byte weekday, byte mon, byte century, byte year)
{
    /* year val is 00 to 99, xx
        with the highest bit of month = century
        0=20xx
        1=19xx
        */
    month = decToBcd(mon);
    if (century == 1){
        month |= RTCC_CENTURY_MASK;
    }
    else {
        month &= ~RTCC_CENTURY_MASK;
    }
    WW.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
    WW.send(RTCC_DAY_ADDR);
    WW.send(decToBcd(day));            //set day
    WW.send(decToBcd(weekday));    //set weekday
    WW.send(month);                         //set month, century to 1
    WW.send(decToBcd(year));        //set year to 99
    WW.endTransmission();
}

/* enable alarm interrupt
 * whenever the clock matches these values an int will
 * be sent out pin 3 of the Pcf8563 chip
 */
void Rtc_Pcf8563::enableAlarm()
{

    //set status2 AF val to zero
    status2 &= ~RTCC_ALARM_AF;
    //enable the interrupt
    status2 |= RTCC_ALARM_AIE;

    //enable the interrupt
    WW.beginTransmission(Rtcc_Addr);  // Issue I2C start signal
    WW.send(RTCC_STAT2_ADDR);
    WW.send(status2);
    WW.endTransmission();
}


/*
* Read status byte
* internal usage only here. see alarmEnabled, alarmActive.
*/
byte Rtc_Pcf8563::readStatus2()
{
    /* set the start byte of the status 2 data */
    WW.beginTransmission(Rtcc_Addr);
    WW.send(RTCC_STAT2_ADDR);
    WW.endTransmission();

    WW.requestFrom(Rtcc_Addr, 1); //request 1 bytes
    return WW.read();
}

/*
* Returns true if AIE is on
*
*/
boolean Rtc_Pcf8563::alarmEnabled()
{
    return Rtc_Pcf8563::readStatus2() & RTCC_ALARM_AIE;
}

/*
* Returns true if AF is on
*
*/
boolean Rtc_Pcf8563::alarmActive()
{
    return Rtc_Pcf8563::readStatus2() & RTCC_ALARM_AF;
}


/* set the alarm values
 * whenever the clock matches these values an int will
 * be sent out pin 3 of the Pcf8563 chip
 */
void Rtc_Pcf8563::setAlarm(byte min, byte hour, byte day, byte weekday)
{
    if (min <99) {
        min = constrain(min, 0, 59);
        min = decToBcd(min);
        min &= ~RTCC_ALARM;
    } else {
        min = 0x0; min |= RTCC_ALARM;
    }

    if (hour <99) {
        hour = constrain(hour, 0, 23);
        hour = decToBcd(hour);
        hour &= ~RTCC_ALARM;
    } else {
        hour = 0x0; hour |= RTCC_ALARM;
    }

    if (day <99) {
        day = constrain(day, 1, 31);
        day = decToBcd(day); day &= ~RTCC_ALARM;
    } else {
        day = 0x0; day |= RTCC_ALARM;
    }

    if (weekday <99) {
        weekday = constrain(weekday, 0, 6);
        weekday = decToBcd(weekday);
        weekday &= ~RTCC_ALARM;
    } else {
        weekday = 0x0; weekday |= RTCC_ALARM;
    }

    Rtc_Pcf8563::enableAlarm();

    //TODO bounds check  the inputs first
    WW.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
    WW.send(RTCC_ALRM_MIN_ADDR);
    WW.send(min);                //minute alarm value reset to 00
    WW.send(hour);                //hour alarm value reset to 00
    WW.send(day);                //day alarm value reset to 00
    WW.send(weekday);            //weekday alarm value reset to 00
    WW.endTransmission();
}

/**
* Get alarm, set values to RTCC_NO_ALARM (99) if alarm flag is not set
*/
void Rtc_Pcf8563::getAlarm()
{
    /* set the start byte of the alarm data */
    WW.beginTransmission(Rtcc_Addr);
    WW.send(RTCC_ALRM_MIN_ADDR);
    WW.endTransmission();

    WW.requestFrom(Rtcc_Addr, 4); //request 4 bytes
    alarm_minute = WW.read();
    if(B10000000 & alarm_minute){
        alarm_minute = RTCC_NO_ALARM;
    } else {
        alarm_minute = bcdToDec(alarm_minute & B01111111);
    }
    alarm_hour = WW.read();
    if(B10000000 & alarm_hour){
        alarm_hour = RTCC_NO_ALARM;
    } else {
        alarm_hour = bcdToDec(alarm_hour & B00111111);
    }
    alarm_day = WW.read();
    if(B10000000 & alarm_day){
        alarm_day = RTCC_NO_ALARM;
    } else {
        alarm_day = bcdToDec(alarm_day  & B00111111);
    }
    alarm_weekday = WW.read();
    if(B10000000 & alarm_weekday){
        alarm_weekday = RTCC_NO_ALARM;
    } else {
        alarm_weekday = bcdToDec(alarm_weekday  & B00000111);
    }
}

/**
* Set the square wave pin output
*/
void Rtc_Pcf8563::setSquareWave(byte frequency)
{
    WW.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
    WW.send(RTCC_SQW_ADDR);
    WW.send(frequency);
    WW.endTransmission();
}

void Rtc_Pcf8563::clearSquareWave()
{
    Rtc_Pcf8563::setSquareWave(SQW_DISABLE);
}

/**
* Reset the alarm leaving interrupt unchanged
*/
void Rtc_Pcf8563::resetAlarm()
{
    //set status2 AF val to zero to reset alarm
    status2 &= ~RTCC_ALARM_AF;
    WW.beginTransmission(Rtcc_Addr);
    WW.send(RTCC_STAT2_ADDR);
    WW.send(status2);
    WW.endTransmission();
}


void Rtc_Pcf8563::clearAlarm()
{
    //set status2 AF val to zero to reset alarm 
    status2 &= ~RTCC_ALARM_AF;
    //turn off the interrupt
    status2 &= ~RTCC_ALARM_AIE;
        
  WW.beginTransmission(Rtcc_Addr);
    WW.send(RTCC_STAT2_ADDR);
  WW.send(status2); 
  WW.endTransmission();  
}

/* call this first to load current date values to variables */
void Rtc_Pcf8563::getDate()
{  
    /* set the start byte of the date data */
    WW.beginTransmission(Rtcc_Addr);
    WW.send(RTCC_DAY_ADDR);
    WW.endTransmission();
    
    WW.requestFrom(Rtcc_Addr, 4); //request 4 bytes
    //0x3f = 0b00111111
    day = bcdToDec(WW.receive() & 0x3f);
    //0x07 = 0b00000111
    weekday = bcdToDec(WW.receive() & 0x07);
    //get raw month data byte and set month and century with it.
    month = WW.receive();
    if (month & RTCC_CENTURY_MASK) {
        century = 1;
    }
    else {
        century = 0;
    }
    //0x1f = 0b00011111
    month = month & 0x1f;
    month = bcdToDec(month);
    year = bcdToDec(WW.receive());  
}


/* call this first to load current time values to variables */
void Rtc_Pcf8563::getTime()
{  
    /* set the start byte , get the 2 status bytes */
    WW.beginTransmission(Rtcc_Addr);
    WW.send(RTCC_STAT1_ADDR);
    WW.endTransmission();
    
    WW.requestFrom(Rtcc_Addr, 5); //request 5 bytes
    status1 = WW.receive();
    status2 = WW.receive();
    //0x7f = 0b01111111
    sec = bcdToDec(WW.receive() & 0x7f);
    minute = bcdToDec(WW.receive() & 0x7f);
    //0x3f = 0b00111111
    hour = bcdToDec(WW.receive() & 0x3f);
}

char const *Rtc_Pcf8563::version(){
  return RTCC_VERSION;  
}

char *Rtc_Pcf8563::formatTime(byte style)
{
    getTime();
    switch (style) {
        case RTCC_TIME_HM:
            strOut[0] = '0' + (hour / 10);
            strOut[1] = '0' + (hour % 10);
            strOut[2] = ':';
            strOut[3] = '0' + (minute / 10);
            strOut[4] = '0' + (minute % 10);
            strOut[5] = '\0';
            break;
        case RTCC_TIME_HMS:
        default:
            strOut[0] = '0' + (hour / 10);
            strOut[1] = '0' + (hour % 10);
            strOut[2] = ':';
            strOut[3] = '0' + (minute / 10);
            strOut[4] = '0' + (minute % 10);
            strOut[5] = ':';
            strOut[6] = '0' + (sec / 10);
            strOut[7] = '0' + (sec % 10);
            strOut[8] = '\0';
            break;
        }
    return strOut;
}
 

char *Rtc_Pcf8563::formatDate(byte style)
{
    getDate();
    
        switch (style) {
        
        case RTCC_DATE_ASIA:
            //do the asian style, yyyy-mm-dd
            if ( century == 1 ){
                strDate[0] = '1';
                strDate[1] = '9';
            }
            else {
                strDate[0] = '2';
                strDate[1] = '0';
            }
            strDate[2] = '0' + (year / 10 );
            strDate[3] = '0' + (year % 10);
            strDate[4] = '-';   
            strDate[5] = '0' + (month / 10);
            strDate[6] = '0' + (month % 10);
            strDate[7] = '-';
            strDate[8] = '0' + (day / 10);
            strDate[9] = '0' + (day % 10);
            strDate[10] = '\0';
            break;
        case RTCC_DATE_US: 
      //the pitiful US style, mm/dd/yyyy
            strDate[0] = '0' + (month / 10);
            strDate[1] = '0' + (month % 10);
            strDate[2] = '/';
            strDate[3] = '0' + (day / 10);
            strDate[4] = '0' + (day % 10);
            strDate[5] = '/';
            if ( century == 1 ){
                strDate[6] = '1';
                strDate[7] = '9';
            }
            else {
                strDate[6] = '2';
                strDate[7] = '0';
            }
            strDate[8] = '0' + (year / 10 );
            strDate[9] = '0' + (year % 10);
            strDate[10] = '\0';
            break;
        case RTCC_DATE_WORLD:
        default:
            //do the world style, dd-mm-yyyy
            strDate[0] = '0' + (day / 10);
            strDate[1] = '0' + (day % 10);
            strDate[2] = '-';
            strDate[3] = '0' + (month / 10);
            strDate[4] = '0' + (month % 10);
            strDate[5] = '-';
            
            if ( century == 1 ){
                strDate[6] = '1';
                strDate[7] = '9';
            }
            else {
                strDate[6] = '2';
                strDate[7] = '0';
            }
            strDate[8] = '0' + (year / 10 );
            strDate[9] = '0' + (year % 10);
            strDate[10] = '\0';
            break;  
        
    }
    return strDate;
}

byte Rtc_Pcf8563::getSecond() {   
  getTime();
    return sec; 
}

byte Rtc_Pcf8563::getMinute() {
  getTime();
    return minute;
}

byte Rtc_Pcf8563::getHour() {
  getTime();
  return hour;
}

byte Rtc_Pcf8563::getAlarmMinute() {
    getAlarm();
    return alarm_minute;
}

byte Rtc_Pcf8563::getAlarmHour() {
    getAlarm();
    return alarm_hour;
}

byte Rtc_Pcf8563::getAlarmDay() {
    getAlarm();
    return alarm_day;
}

byte Rtc_Pcf8563::getAlarmWeekday() {
    getAlarm();
    return alarm_weekday;
}

byte Rtc_Pcf8563::getDay() {
  getDate();
    return day;
}

byte Rtc_Pcf8563::getMonth() {
    getDate();
  return month;
}

byte Rtc_Pcf8563::getYear() {
  getDate();
    return year;
}

byte Rtc_Pcf8563::getWeekday() {
  getDate();
    return weekday;
}

byte Rtc_Pcf8563::getStatus1() {
    return status1;
}

byte Rtc_Pcf8563::getStatus2() {
    return status2;
}
