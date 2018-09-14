#include <CurrentTransformer.h>     // https://github.com/JChristensen/CurrentTransformer
#include <LiquidTWI.h>              // https://forums.adafruit.com/viewtopic.php?t=21586
#include <Wire.h>

CT_Sensor ct0(A0, 1000, 200);

// An I2C LCD class that doesn't hang if an LCD is not connected.
// LCD presence is detected when begin() is called. Subsequent calls
// to clear(), setCursor() and write() communicate to the LCD or not
// based on the results of the detection in begin().

class OptionalLCD : public LiquidTWI
{
    public:
        OptionalLCD(uint8_t i2cAddr) : LiquidTWI(i2cAddr), m_i2cAddr(i2cAddr + 0x20) {}
        void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);
        void clear();
        void setCursor(uint8_t col, uint8_t row);
        virtual size_t write(uint8_t value);
        bool isPresent() {return m_lcdPresent;}

    private:
        uint8_t m_i2cAddr;
        bool m_lcdPresent;
};

void OptionalLCD::begin(uint8_t cols, uint8_t rows, uint8_t charsize)
{
    Wire.begin();
    Wire.beginTransmission(m_i2cAddr);
    uint8_t s = Wire.endTransmission();     // status is zero if successful
    m_lcdPresent = (s == 0);
    if (m_lcdPresent) LiquidTWI::begin(cols, rows, charsize);
}

void OptionalLCD::clear()
{
    if (m_lcdPresent) LiquidTWI::clear();
}

void OptionalLCD::setCursor(uint8_t col, uint8_t row)
{
    if (m_lcdPresent) LiquidTWI::setCursor(col, row);
}

size_t OptionalLCD::write(uint8_t value)
{
    if (m_lcdPresent)
        return LiquidTWI::write(value);
    else
        return 0;
}

OptionalLCD lcd(0);     // i2c address 0 (0x20)

class CurrentSensor : public CT_Control
{
    public:
        CurrentSensor(uint32_t threshold, int8_t led = -1);
        void begin();
        void restart();
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

// configure led and lcd hardware, initialize CT_Control
void CurrentSensor::begin()
{
    if (m_led >= 0) pinMode(m_led, OUTPUT);
    lcd.begin(16, 2);
    if (lcd.isPresent())
        Serial << millis() << F(" LCD detected\n");
    else
        Serial << millis() << F(" LCD not present\n");
    lcd.clear();
    restart();
}

// initialize CT_Control, display vcc value
void CurrentSensor::restart()
{
    float vcc = CT_Control::begin();
    lcd.setCursor(0, 1);
    lcd << F("VCC  " ) << _FLOAT(vcc, 3) << F(" V ");
    Serial << millis() << F(" Vcc ") << _FLOAT(vcc, 3) << endl;
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
    lcd << F("CT-0 " ) << _FLOAT(a, 3) << F(" A ");
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

