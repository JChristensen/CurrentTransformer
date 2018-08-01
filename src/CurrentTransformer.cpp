// Arduino Current Transformer Library
// https://github.com/JChristensen/CurrentTransformer
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#include <CurrentTransformer.h>

CurrentTransformer::CurrentTransformer(uint8_t adcChannel, float turnsRatio,
    float rBurden, float vcc, ctFreq_t freq)
    : m_ratio(turnsRatio), m_rBurden(rBurden), m_vcc(vcc)
{
    if (adcChannel >= 14) adcChannel -= 14; // if user passed A0-A5, adjust accordingly
    m_channel = adcChannel & 0x07;          // ruthlessly coerce to an acceptable value
    tcOCR1 = (freq == CT_FREQ_50HZ) ? 4922 : 4102;
}

volatile bool CurrentTransformer::adcBusy;
volatile int CurrentTransformer::adcVal;
const int CurrentTransformer::sampleSize(65);   // number of samples to cover one cycle

void CurrentTransformer::begin()
{
    // set up the timer
    TCCR1B = 0;                 // stop the timer
    TCCR1A = 0;
    TIFR1 = 0xFF;               // ensure all interrupt flags are cleared
    OCR1A = tcOCR1;             // set timer output compare value
    OCR1B = tcOCR1;
    cli();
    TCNT1 = 0;                  // clear the timer
    TIMSK1 = _BV(OCIE1B);       // enable timer interrupts
    sei();
    TCCR1B = _BV(WGM12) | _BV(CS10);    // start the timer, ctc mode, prescaler divide by 1

    // set up the adc
    ADMUX = _BV(REFS0) | m_channel;                     // set channel, and AVcc as reference
    ADCSRA  = _BV(ADEN)  | _BV(ADATE) | _BV(ADIE);      // enable ADC, auto trigger, interrupt when conversion complete
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);     // ADC clock prescaler: divide by 128 (for 125kHz)
    ADCSRB = _BV(ADTS2) | _BV(ADTS0);                   // trigger ADC on Timer/Counter1 Compare Match B    
}

float CurrentTransformer::read()
{
    uint8_t n(0);                       // sample count
    int32_t sumsq(0);                   // sum of squares
    while (adcBusy);                    // if a conversion is in progress, wait for it to complete
    ADMUX = _BV(REFS0) | m_channel;     // set channel, and AVcc as reference

    do {
        while (!adcBusy);               // wait for next conversion to start
        while (adcBusy);                // wait for conversion to complete
        cli();
        int32_t v = adcVal;             // get the reading, promote to 32 bit
        sei();
        // accumulate the sum of squares,
        // subtract 512 (half the adc range) to remove dc component
        sumsq += (v - 512) * (v - 512);
    } while (++n < sampleSize);
    
    // calculate rms voltage and current
    float Vrms = m_vcc * sqrt(static_cast<float>(sumsq) / static_cast<float>(sampleSize - 1)) / 1024;
    return m_ratio * Vrms / m_rBurden;
}

// adc conversion complete, pass the value back to the main code
ISR(ADC_vect)
{
    CurrentTransformer::adcBusy = false;
    CurrentTransformer::adcVal = ADC;
}

// adc starts conversion when the timer interrupt fires
ISR(TIMER1_COMPB_vect)
{
    CurrentTransformer::adcBusy = true;
}

