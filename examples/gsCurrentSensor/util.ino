// print date and time to Serial
void printDateTime(time_t t)
{
    printDate(t);
    printTime(t);
    Serial << tcr -> abbrev << endl;
}

// print date to Serial
void printDate(time_t t)
{
    Serial << year(t) << '-';
    printI00(month(t), '-');
    printI00(day(t), ' ');
}

// print time to Serial
void printTime(time_t t)
{
    printI00(hour(t), ':');
    printI00(minute(t), ':');
    printI00(second(t), ' ');
}

// Print an integer in "00" format (with leading zero),
// followed by a delimiter character to Serial.
// Input value assumed to be between 0 and 99.
void printI00(int val, char delim)
{
    if (val < 10) Serial << '0';
    Serial << val << delim;
    return;
}

// calculate the next time where seconds = 0
time_t nextMinute()
{
    tmElements_t tm;

    breakTime(now(), tm);
    tm.Second = 0;
    return makeTime(tm) + 60;
}

