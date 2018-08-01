// Arduino Current Transformer Library
// https://github.com/JChristensen/CurrentTransformer
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#ifndef CURRENT_TRANSFORMER_H_INCLUDED
#define CURRENT_TRANSFORMER_H_INCLUDED
#include <Arduino.h>

// Line frequencies
enum ctFreq_t {CT_FREQ_50HZ, CT_FREQ_60HZ};

class CurrentTransformer
{
    public:
        CurrentTransformer(uint8_t adcChannel, float turnsRatio,
            float rBurden, float vcc=5.0, ctFreq_t freq=CT_FREQ_60HZ);
        void begin();                   // initializations
        float read();                   // read the rms value of one cycle
        static volatile bool adcBusy;   // adc busy flag
        static volatile int adcVal;     // value returned from adc
        static const int sampleSize;    // number of samples to cover one cycle

    private:
        uint8_t m_channel;              // adc channel
        float m_ratio;                  // current transformer turns ratio
        float m_rBurden;                // current transformer burden resistor value, ohms
        float m_vcc;                    // mcu supply voltage
        uint16_t tcOCR1;                // compare value for timer
};

#endif

