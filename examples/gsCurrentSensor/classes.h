#include <CurrentTransformer.h>             // https://github.com/JChristensen/CurrentTransformer
#include <LiquidTWI.h>                 //http://forums.adafruit.com/viewtopic.php?t=21586

CT_Sensor ct0(A0, 1000, 200);
LiquidTWI lcd(0);   //i2c address 0 (0x20)

class CurrentSensor : public CT_Control
{
    public:
        CurrentSensor(uint32_t threshold);
        void begin();
        float sample();
        void clearSampleData();
        int readVcc();
        
        int nSample;            // number of times CT was read
        int nRunning;           // number of times current was >= threshold value
        // current values in milliamps (mA):
        uint32_t maThreshold;   // current must equal or exceed this value to be counted as "running"
        uint32_t maSum;         // sum of all current samples
        uint32_t maMin;         // smallest current observed
        uint32_t maMax;         // largest current observed
};

CurrentSensor::CurrentSensor(uint32_t threshold) : maThreshold(threshold)
{
    clearSampleData();
}

void CurrentSensor::begin()
{
    CT_Control::begin(static_cast<float>(readVcc()) / 1000.0);
    lcd.begin(16, 2);
    lcd.clear();
}

// read the ct and collect sample data, display on lcd
float CurrentSensor::sample()
{
    read(&ct0);
    float a = ct0.amps();
    uint32_t ma = a * 1000.0 + 0.5;
    ++nSample;
    if (ma >= maThreshold)
    {
        ++nRunning;
        maSum += ma;
        if (ma < maMin) maMin = ma;
        if (ma > maMax) maMax = ma;
    }
    lcd.setCursor(0, 0);
    lcd << F("CT-0 " ) << _FLOAT(a, 3) << F(" AMP ");
    return a;
}

void CurrentSensor::clearSampleData()
{
    nSample = 0;
    nRunning = 0;
    maSum = 0;
    maMax = 0;
    maMin = 999999;
}

// read 1.1V reference against AVcc
// from http://code.google.com/p/tinkerit/wiki/SecretVoltmeter
int CurrentSensor::readVcc()
{
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(5);                               // Vref settling time
    ADCSRA |= _BV(ADSC);                    // start conversion
    loop_until_bit_is_clear(ADCSRA, ADSC);  // wait for it to complete
    delay(5);
    ADCSRA |= _BV(ADSC);                    // start conversion
    loop_until_bit_is_clear(ADCSRA, ADSC);  // wait for it to complete
    return 1125300L / ADC;                  // calculate AVcc in mV (1.1 * 1000 * 1023)
}

