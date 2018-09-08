// Arduino Current Transformer Library
// https://github.com/JChristensen/CurrentTransformer
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#ifndef CURRENT_TRANSFORMER_H_INCLUDED
#define CURRENT_TRANSFORMER_H_INCLUDED
#include <Arduino.h>

// Line frequencies
enum ctFreq_t {CT_FREQ_50HZ, CT_FREQ_60HZ};

class CT_Sensor
{
    public:
        CT_Sensor(uint8_t channel, float ratio, float burden);
        float amps() {return m_amps;}

    protected:
        uint8_t m_channel;                  // adc channel number
        float m_ratio;                      // ct turns ratio
        float m_burden;                     // ct burden resistor, ohms
        float m_amps;                       // rms amperes measured

    friend class CT_Control;
};

class CT_Control
{
    public:
        CT_Control(ctFreq_t freq=CT_FREQ_60HZ);
        float begin();                      // initializations; returns Vcc
        void end();                         // reset adc and timer to defaults
        // read the rms value of one cycle for one or two CTs
        void read(CT_Sensor *ct0, CT_Sensor *ct1);
        void read(CT_Sensor *ct0) {read(ct0, ct0);}
        static volatile bool adcBusy;       // adc busy flag
        static volatile int adcVal;         // value returned from adc
        static const uint16_t sampleSize;   // number of samples to cover one cycle
        static const uint16_t ADC_MAX;      // maximum adc reading
        static const uint16_t OCR50;        // timer output compare register value for 50Hz
        static const uint16_t OCR60;        // timer output compare register value for 60Hz

    private:
        float m_vcc;                        // mcu supply voltage
        uint16_t m_tcOCR1;                  // compare value for timer
};

#endif
