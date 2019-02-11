// Arduino Current Transformer Library
// https://github.com/JChristensen/CurrentTransformer
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// Example sketch to read a current transformer and send the
// data to GroveStreams. The CT is measured once per second and data
// is sent to GroveStreams every 10 minutes. The current measurement
// is also displayed on a serial (I2C) LCD display.
// Tested with TA17L-03 current transformer (10A max), Arduino Uno,
// Arduino v1.8.5.

#include <gsXBee.h>             // https://github.com/JChristensen/gsXBee
#include <Streaming.h>          // http://arduiniana.org/libraries/streaming/
#include <TimeLib.h>            // https://github.com/PaulStoffregen/Time
#include <Timezone.h>           // https://github.com/JChristensen/Timezone
#include <XBee.h>               // https://github.com/andrewrapp/xbee-arduino
#include "classes.h"

// pin definitions and other constants
const uint8_t
    xbeeReset(4),
    grnLED(11),                         // heartbeat
    yelLED(12),                         // pump on
    redLED(13),                         // wait for time sync
    unusedPins[] = {2, 3, 5, 6, 7, 8, 9, 10, A2, A3};
const int32_t BAUD_RATE(115200);
const uint32_t XBEE_TIMEOUT(3000);      // max wait time for ack, ms
const int SYNC_MINUTE(58);              // hourly time sync minute. sync occurs after regular data
                                        // transmission on or after this minute.
const time_t SYNC_INTERVAL(60*60);      // time sync interval, sec
const time_t SYNC_RETRY_INTERVAL(5*60); // time sync retry interval, sec

// object instantiations
gsXBee xb;                              // the XBee
CurrentSensor cs(150, yelLED);          // current transformer, 150mA threshold
heartbeat hbLED(grnLED, 1000);

// variables for time and timing
time_t utc;                             // current utc time
time_t utcStart;                        // sketch start time (actually the first time sync received)
time_t lastTimeSyncRecd;                // last time sync received
time_t nextWebTx;                       // time for next web data transmission
time_t nextTimeSync;                    // time for next time sync
time_t timeSyncRetry;                   // for time sync retries 

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};  // Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};   // Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;                    // pointer to the time change rule, use to get TZ abbrev

// other global variables
XBeeAddress64 coordinator(0x0, 0x0);    // coordinator address

void setup()
{
    Serial.begin(BAUD_RATE);
    Serial << F( "\n" __FILE__ " " __DATE__ " " __TIME__ "\n" );
    // enable pullups on unused pins for noise immunity
    for ( uint8_t i=0; i<sizeof(unusedPins); ++i )
        pinMode(unusedPins[i], INPUT_PULLUP);

    pinMode(redLED, OUTPUT);
    pinMode(yelLED, OUTPUT);
    hbLED.begin();
    pinMode(xbeeReset, OUTPUT);         // drives pin low to reset the XBee
    digitalWrite(redLED, HIGH);         // lamp test
    digitalWrite(yelLED, HIGH);
    digitalWrite(grnLED, HIGH);
    delay(1000);
    digitalWrite(redLED, LOW);
    digitalWrite(yelLED, LOW);
    digitalWrite(grnLED, LOW);
    digitalWrite(xbeeReset, HIGH);      // let the XBee initialize
}

//state machine states
enum STATES_t
{
    XBEE_INIT, REQ_TIMESYNC, WAIT_TIMESYNC, RUN
}
STATE;

void loop()
{
    static time_t nextTimePrint;                // next time to print the local time to serial

    utc = now();
    xbeeReadStatus_t xbStatus = xb.read();      // check for incoming XBee traffic
    hbLED.update();

    switch (STATE)
    {
    case XBEE_INIT:
        if ( xb.begin(Serial) )
        {
            STATE = REQ_TIMESYNC;
            xb.setSyncCallback(processTimeSync);
        }
        else
        {
            Serial << millis() << F(" XBee init fail\n");
            xb.mcuReset(10000);
        }
        break;

    case REQ_TIMESYNC:
        STATE = WAIT_TIMESYNC;
        digitalWrite(redLED, HIGH);
        xb.destAddr = coordinator;
        xb.requestTimeSync(utc);
        break;

    case WAIT_TIMESYNC:
        if (lastTimeSyncRecd > 0)
        {
            STATE = RUN;
            nextTimePrint = nextMinute();
            utcStart = lastTimeSyncRecd;
            nextWebTx = utc - utc % (xb.txInterval * 60) + xb.txOffset * 60 + xb.txSec - xb.txWarmup;
            if ( nextWebTx <= utc + 5 ) nextWebTx += xb.txInterval * 60;

            // calculate time for next time sync
            tmElements_t tm;
            breakTime(utc, tm);
            tm.Minute = SYNC_MINUTE;
            tm.Second = xb.txSec;
            nextTimeSync = makeTime(tm);
            if ( nextTimeSync <= utc ) nextTimeSync += SYNC_INTERVAL;

            // print the first time sync
            time_t local = myTZ.toLocal(utc, &tcr);     // TZ adjustment
            Serial << millis() << ' ';
            printDateTime(local);
            cs.begin();
        }
        else if (millis() - xb.msTX >= XBEE_TIMEOUT)    // timeout waiting for time sync response
        {
            STATE = REQ_TIMESYNC;
        }
        break;

    case RUN:
        xmit(xbStatus);                 // run the transmit state machine

        // once-per-second processing
        static time_t lastUTC;          // last time one-second processing code was executed
        if (utc > lastUTC)
        {
            lastUTC = utc;
            cs.sample();

            if (utc >= nextTimePrint)   // print time to Serial once per minute
            {
                nextTimePrint += 60;
                time_t local = myTZ.toLocal(utc, &tcr);     // TZ adjustment
                Serial << millis() << ' ';
                printDateTime(local);
                cs.end();               // stop & restart the current sensor to re-read Vcc
                cs.restart();
            }
        }
        break;
    }
}

// transmit state machine sequences XBee transmissions (data and time sync requests) for
// effective use of the transmit window.

// macro to concatenate an identifier and value to the API URL
#define payloadInteger(id, value) {char cTemp[12]; strcat(payload, "&" #id "="); ltoa(value, cTemp, 10); strcat(payload, cTemp);}

// transmit machine states
enum TX_STATES_t
{
    TX_WAIT, TX_SEND_WEB_DATA, TX_WAIT_WEB_ACK, TX_SEND_SYNC_REQ, TX_WAIT_SYNC_ACK
}
TX_STATE;

void xmit(xbeeReadStatus_t xbStatus)
{
    switch (TX_STATE)
    {
    static time_t lastUTC;
    static uint16_t seq;        // gs data packet sequence number
    
    case TX_WAIT:               // wait for next second
        if ( utc > lastUTC )
        {
            lastUTC = utc;
            TX_STATE = TX_SEND_WEB_DATA;
        }
        break;

    case TX_SEND_WEB_DATA:
        if ( utc >= nextWebTx )     // time to send data?
        {
            nextWebTx = utc - utc % (xb.txInterval * 60) + xb.txOffset * 60 + xb.txSec - xb.txWarmup;
            if ( nextWebTx <= utc + 5 ) nextWebTx += xb.txInterval * 60;
            char payload[80];
            payload[0] = 0;
            payloadInteger(s, seq++);               // sequence number
            payloadInteger(u, utc - utcStart);      // uptime in seconds
            payloadInteger(n, cs.nSample);
            payloadInteger(r, cs.nRunning);
            payloadInteger(t, cs.maSum);
            // if no current observed, send 0 for the min instead of 999999
            payloadInteger(m, (cs.maMax == 0 ? 0 : cs.maMin));
            payloadInteger(x, cs.maMax);
            xb.destAddr = coordinator;
            xb.sendData(payload);
            cs.clearSampleData();
            TX_STATE = TX_WAIT_WEB_ACK;
            //Serial << millis() << ' ' << cs.nSample << ' ' << cs.nRunning << ' ' << cs.maSum << ' ' << cs.maMin << ' ' << cs.maMax << endl;
        }
        else                        // not time to send data
        {
            TX_STATE = TX_WAIT;
        }
        break;
        
    case TX_WAIT_WEB_ACK:
        if (xbStatus == TX_ACK)
        {
            TX_STATE = TX_SEND_SYNC_REQ;
        }
        else if ( millis() - xb.msTX >= XBEE_TIMEOUT )
        {
            Serial << millis() << F(" XB ACK TIMEOUT\n");
            TX_STATE = TX_SEND_SYNC_REQ;
        }
        break;

    case TX_SEND_SYNC_REQ:
            if ( utc >= nextTimeSync + timeSyncRetry )  // is it time to request a time sync?
            {
                digitalWrite(redLED, HIGH);
                // add short delay for debugging.
                // suspect that the time sync request arrives too soon and the
                // base station reads it as the response to the DB command
                // used to get last hop RSS. was seeing RSS UNEXP RESP messages.
                delay(10);
                timeSyncRetry += SYNC_RETRY_INTERVAL;
                xb.destAddr = coordinator;
                xb.requestTimeSync(utc);
                TX_STATE = TX_WAIT_SYNC_ACK;
            }
            else                                        // no, keep waiting
            {
                TX_STATE = TX_WAIT;
            }
        break;

    case TX_WAIT_SYNC_ACK:
        if (xbStatus == TX_ACK)
        {
            TX_STATE = TX_WAIT;
        }
        else if ( millis() - xb.msTX >= XBEE_TIMEOUT )
        {
            Serial << millis() << F(" XB ACK TIMEOUT\n");
            TX_STATE = TX_WAIT;
        }
        break;
    }
}

// set time to value received from master clock, update time variables
void processTimeSync(time_t t)
{
    setTime(t);
    utc = now();
    if (lastTimeSyncRecd == 0) nextTimeSync = utc;
    while (nextTimeSync <= utc) nextTimeSync += SYNC_INTERVAL;
    lastTimeSyncRecd = t;
    timeSyncRetry = 0;
    digitalWrite(redLED, LOW);
}

