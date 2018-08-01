// Arduino Current Transformer Library
// https://github.com/JChristensen/CurrentTransformer
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// Example sketch to read a current transformer every five seconds
// and display the measurement on a serial (I2C) LCD display.
// Tested with TA17L-03 current transformer (10A max), Arduino Uno,
// Arduino v1.8.5.

#include <CurrentTransformer.h>             // https://github.com/JChristensen/CurrentTransformer
#include <Streaming.h>                      // http://arduiniana.org/libraries/streaming/
#include <LiquidTWI.h>                 //http://forums.adafruit.com/viewtopic.php?t=21586
// or http://dl.dropboxusercontent.com/u/35284720/postfiles/LiquidTWI-1.5.1.zip

const uint8_t ctChannel(0);                 // adc channel
const float ctRatio(1000);                  // current transformer winding ratio
const float rBurden(200);                   // current transformer burden resistor value
const float vcc(5.070);                     // adjust to actual value for best accuracy
const uint32_t MS_BETWEEN_SAMPLES(1000);    // milliseconds
const int32_t BAUD_RATE(115200);

// object definitions
CurrentTransformer ct0(ctChannel, ctRatio, rBurden, vcc);
LiquidTWI lcd(0); //i2c address 0 (0x20)

void setup()
{
    delay(1000);
    Serial.begin(BAUD_RATE);
    ct0.begin();
    lcd.begin(16, 2);
    lcd.clear();
}

void loop()
{
    uint32_t msStart = millis();
    float i0 = ct0.read();
    Serial << millis() << ' ' << _FLOAT(i0, 3) << F(" A\n");
    lcd.setCursor(0, 0);
    lcd << F("CT-") << ctChannel << ' ' << _FLOAT(i0, 3) << F(" AMP");
    while (millis() - msStart < MS_BETWEEN_SAMPLES);  // wait to start next measurement
}
