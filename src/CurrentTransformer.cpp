// Arduino Current Transformer Library
// https://github.com/JChristensen/CurrentTransformer
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#include <CurrentTransformer.h>

CT_Sensor::CT_Sensor(uint8_t channel, float ratio, float burden)
    : m_ratio(ratio), m_burden(burden)
{
    if (channel >= 14) channel -= 14;   // if user passed A0-A5, adjust accordingly
    m_channel = channel & 0x07;         // ruthlessly coerce to an acceptable value
}

volatile bool CT_Control::adcBusy;
volatile int CT_Control::adcVal;
const uint16_t CT_Control::sampleSize(65);   // number of samples to cover one cycle
const uint16_t CT_Control::ADC_MAX(1023);
const uint16_t CT_Control::OCR50(F_CPU / 50 / CT_Control::sampleSize / 2 - 1);
const uint16_t CT_Control::OCR60(F_CPU / 60 / CT_Control::sampleSize / 2 - 1);

CT_Control::CT_Control(ctFreq_t freq)
{
    // save appropriate timer output compare value
    m_tcOCR1 = (freq == CT_FREQ_50HZ) ? CT_Control::OCR50 : CT_Control::OCR60;
}

// configure the timer and adc. reads and returns Vcc value in volts.
float CT_Control::begin()
{
    // read Vcc
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);  // default adc configuration
    ADCSRB = 0;
    // set AVcc as reference, 1.1V bandgap reference voltage as input
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(10);                              // Vref settling time
    ADCSRA |= _BV(ADSC);                    // start conversion
    loop_until_bit_is_clear(ADCSRA, ADSC);  // wait for it to complete
    int mv = 1125300L / ADC;                // calculate AVcc in mV (1.1 * 1000 * 1023)
    m_vcc = static_cast<float>(mv) / 1000.0;
    
    // set up the timer
    TCCR1B = 0;                             // stop the timer
    TCCR1A = 0;
    TIFR1 = 0xFF;                           // ensure all interrupt flags are cleared
    OCR1A = m_tcOCR1;                       // set timer output compare value
    OCR1B = m_tcOCR1;
    cli();
    TCNT1 = 0;                              // clear the timer
    TIMSK1 = _BV(OCIE1B);                   // enable timer interrupts
    sei();
    TCCR1B = _BV(WGM12) | _BV(CS10);    // start the timer, ctc mode, prescaler divide by 1

    // set up the adc
    ADCSRA  = _BV(ADEN)  | _BV(ADATE) | _BV(ADIE);      // enable ADC, auto trigger, interrupt when conversion complete
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);     // ADC clock prescaler: divide by 128 (for 125kHz)
    ADCSRB = _BV(ADTS2) | _BV(ADTS0);                   // trigger ADC on Timer/Counter1 Compare Match B    

    return m_vcc;
}

void CT_Control::end()
{
    // reset adc to default configuration (enabled, clock prescaler 128)
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
    ADCSRB = 0;

    // stop the timer
    TCCR1B = 0;
    TCCR1A = 0;
    TIFR1 = 0xFF;               // ensure all interrupt flags are cleared
}

void CT_Control::read(CT_Sensor *ct0, CT_Sensor *ct1)
{
    uint8_t n(0);                       // sample count
    int32_t sumsq0(0), sumsq1(0);       // sum of squares
    ADMUX = _BV(REFS0) | ct0->m_channel; // set channel, and AVcc as reference
    while (!adcBusy);                   // wait for one conversion
    while (adcBusy);

    do
    {
        // read ct0
        ADMUX = _BV(REFS0) | ct0->m_channel;    // set channel, and AVcc as reference
        while (!adcBusy);                       // wait for next conversion to start
        while (adcBusy);                        // wait for conversion to complete
        int32_t v0 = adcVal;                    // get the reading, promote to 32 bit

        // read ct1
        ADMUX = _BV(REFS0) | ct1->m_channel;    // set channel, and AVcc as reference
        while (!adcBusy);                       // wait for next conversion to start
        while (adcBusy);                        // wait for conversion to complete
        int32_t v1 = adcVal;                    // get the reading, promote to 32 bit

        // accumulate the sum of squares,
        // subtract 512 (half the adc range) to remove dc component
        sumsq0 += (v0 - ADC_MAX/2) * (v0 - ADC_MAX/2);
        sumsq1 += (v1 - ADC_MAX/2) * (v1 - ADC_MAX/2);
    } while (++n < sampleSize);
    
    // calculate rms voltage and current
    float Vrms0 = m_vcc * sqrt(static_cast<float>(sumsq0) / static_cast<float>(sampleSize - 1)) / ADC_MAX;
    float Vrms1 = m_vcc * sqrt(static_cast<float>(sumsq1) / static_cast<float>(sampleSize - 1)) / ADC_MAX;
    ct0->m_amps = ct0->m_ratio * Vrms0 / ct0->m_burden;
    ct1->m_amps = ct1->m_ratio * Vrms1 / ct1->m_burden;
    return;
}

// adc conversion complete, pass the value back to the main code
ISR(ADC_vect)
{
    CT_Control::adcBusy = false;
    CT_Control::adcVal = ADC;
}

// adc starts conversion when the timer interrupt fires
ISR(TIMER1_COMPB_vect)
{
    CT_Control::adcBusy = true;
}
