#include <CurrentTransformer.h>     // https://github.com/JChristensen/CurrentTransformer
#include <LiquidTWI.h>              // https://forums.adafruit.com/viewtopic.php?t=21586

CT_Sensor ct0(A0, 1000, 200);
LiquidTWI lcd(0);   //i2c address 0 (0x20)

class CurrentSensor : public CT_Control
{
    public:
        CurrentSensor(uint32_t threshold, int8_t led = -1);
        void begin();
        float sample();
        void clearSampleData();
        
        int nSample;            // number of times CT was read
        int nRunning;           // number of times current was >= threshold value
        // current values in milliamps (mA):
        uint32_t maThreshold;   // current must equal or exceed this value to be counted as "running"
        uint32_t maSum;         // sum of all current samples
        uint32_t maMin;         // smallest current observed
        uint32_t maMax;         // largest current observed

    private:
        int8_t m_led;           // led to indicate device on
};

CurrentSensor::CurrentSensor(uint32_t threshold, int8_t led) : maThreshold(threshold), m_led(led)
{
    clearSampleData();
}

void CurrentSensor::begin()
{
    if (m_led >= 0) pinMode(m_led, OUTPUT);
    float vcc = readVcc();
    CT_Control::begin(vcc);
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd << F("Vcc = ") << _FLOAT(vcc, 3);
    Serial << millis() << F(" Vcc = ") << _FLOAT(vcc, 3) << endl;
    delay(1000);
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
        if (m_led >= 0) digitalWrite(m_led, HIGH);
        ++nRunning;
        maSum += ma;
        if (ma < maMin) maMin = ma;
        if (ma > maMax) maMax = ma;
    }
    else
    {
        if (m_led >= 0) digitalWrite(m_led, LOW);
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

// heartbeat led class
class heartbeat
{
public:
    heartbeat(uint8_t pin, uint32_t interval)
        : m_pin(pin), m_interval(interval), m_state(true) {}
    void begin();
    void update();

private:
    uint8_t m_pin;
    uint32_t m_interval;
    uint32_t m_lastHB;
    bool m_state;
};

void heartbeat::begin()
{
    pinMode(m_pin, OUTPUT);
    digitalWrite(m_pin, m_state);
    m_lastHB = millis();
}

void heartbeat::update()
{
    if ( millis() - m_lastHB >= m_interval )
    {
        m_lastHB += m_interval;
        digitalWrite( m_pin, m_state = !m_state);
    }
}

