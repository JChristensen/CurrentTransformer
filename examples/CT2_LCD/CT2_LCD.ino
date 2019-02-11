// Arduino Current Transformer Library
// https://github.com/JChristensen/CurrentTransformer
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// Example sketch to read two current transformers and display
// the measurements on a serial (I2C) LCD display.
// Tested with YHDC TA17L-03 current transformer (10A max),
// Arduino Uno, Arduino v1.8.5.

#include <CurrentTransformer.h>     // https://github.com/JChristensen/CurrentTransformer
#include <Streaming.h>              // http://arduiniana.org/libraries/streaming/
#include <LiquidTWI.h>              // http://forums.adafruit.com/viewtopic.php?t=21586

const uint8_t ctChannel0(0);                // adc channel for ct-0
const uint8_t ctChannel1(1);                // adc channel for ct-1
const float ctRatio(1000);                  // current transformer winding ratio
const float rBurden(200);                   // current transformer burden resistor value
const uint32_t MS_BETWEEN_SAMPLES(1000);    // milliseconds
const int32_t BAUD_RATE(115200);

// object definitions
CT_Sensor ct0(ctChannel0, ctRatio, rBurden);
CT_Sensor ct1(ctChannel1, ctRatio, rBurden);
CT_Control ct;
LiquidTWI lcd(0);                           // i2c address 0 (0x20)

void setup()
{
    Serial.begin(BAUD_RATE);
    lcd.begin(16, 2);
    lcd.clear();
    delay(1000);
    float vcc = ct.begin();
    lcd.setCursor(0, 0);
    lcd << F("VCC ") << _FLOAT(vcc, 3) << F(" V  ");
    Serial << millis() << F("  VCC ")  << _FLOAT(vcc, 3) << F(" V\n");
    delay(2000);
    lcd.clear();
}

void loop()
{
    uint32_t msStart = millis();
    ct.read(&ct0, &ct1);
    float i0 = ct0.amps();
    float i1 = ct1.amps();
    Serial << millis() << F("  ") << _FLOAT(i0, 3) << F(" A  ") << _FLOAT(i1, 3) << F(" A\n");
    lcd.setCursor(0, 0);
    lcd << F("CT-") << ctChannel0 << ' ' << _FLOAT(i0, 3) << F(" AMP");
    lcd.setCursor(0, 1);
    lcd << F("CT-") << ctChannel1 << ' ' << _FLOAT(i1, 3) << F(" AMP");
    while (millis() - msStart < MS_BETWEEN_SAMPLES);  // wait to start next measurement
}
