# Arduino CurrentTransformer Library
https://github.com/JChristensen/CurrentTransformer  
README file  

## License
Arduino CurrentTransformer Library Copyright (C) 2018 Jack Christensen GNU GPL v3.0

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License v3.0 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/gpl.html>

## Introduction
The CurrentTransformer library measures RMS current values in a 50/60Hz AC circuit using [current transformers](https://en.wikipedia.org/wiki/Current_transformer). Each read causes the analog-to-digital converter (ADC) to measure a single AC cycle. The data gathered is processed using the [standard RMS calculation](https://en.wikipedia.org/wiki/Root_mean_square#Definition) to give the result in amperes.

The library contains two classes. `CT_Sensor` is used to define one or more current transformers. `CT_Control` does the actual measurement when `CT_Control::read()` is called. Each call to `read()` can read either one or two current transformers in a single AC cycle.

Since each call to `read()` measures only a single cycle, it may be desirable, depending on the application and the nature of the electrical load, to make several calls, perhaps averaging the results, removing transients, etc. Each call to `read()` only takes about as long as one AC cycle, so there is not a lot of overhead or delay in taking several measurements.

Because measuring AC causes the current transformer to output positive and negative currents, a DC bias must be applied to ensure that below-ground voltages are not applied to the microcontroller's ADC input. It is also necessary to ensure that peak voltages do not exceed the microcontroller's supply voltage (Vcc). Do the math to ensure that your circuit operates within safe limits; also see the example below.

This library is specific to the AVR microcontroller architecture and will not work on others. Timer/Counter1 is used to trigger the ADC conversions and so is not available for other purposes.

## Example Design Calculations

The TA17L-03 current transformer is rated at 10A maximum and has a 1000:1 turns ratio. A 200Ω burden resistor is recommended. A 10A RMS current in the primary will generate a 10mA current in the secondary and therefore 2V across the burden resistor. However the peak voltage will then be √2 * 2V = ±2.8V (assuming a sine wave) which exceeds the 2.5V DC bias provided by the circuit below. Therefore the measured current should be limited to about 8.5A RMS (giving ±2.4V P-P) or perhaps a smaller burden resistor could be used if larger currents need to be measured.

## Typical Circuit
![](https://raw.githubusercontent.com/JChristensen/CurrentTransformer/master/extras/typical-circuit.png)

## Enumeration
### ctFreq_t
##### Description
Operating frequency for the current transformer.
##### Values
- CT_FREQ_50HZ
- CT_FREQ_60HZ

## Constructors

### CT_Sensor(uint8_t channel, float ratio, float burden)
### CT_Sensor(uint8_t channel, float amps, float volts)
##### Description 
Defines a CT_Sensor object. One or more CT_Sensor objects can be defined as needed.  
The first form is for current transformers with a user-supplied burden resistor. In this case, the turns ratio and the burden resistor value are given.  
The second form is for current transformers with a built-in burden resistor. These are often specified as the output voltage corresponding to the maximum input current, e.g. 20A/1V.
##### Syntax
```c++
CT_Sensor myCT(channel, ratio, burden);
/* or */
CT_Sensor myCT(channel, amps, volts);
```
##### Parameters
**channel:** ADC channel number that the current transformer is connected to. (Arduino pin numbers can also be used, e.g. A0-A5). *(uint8_t)*  
**ratio:** Secondary-to-primary turns ratio for the current transformer. *(float)*  
**burden:** Current transformer burden resistor value in ohms. *(float)*  
**amps:** Maximum rated current for the current transformer. *(float)*  
**volts:** Voltage output corresponding to the maximum current input. *(float)*
##### Example
```c++
CT_Sensor mySensor(0, 1000, 200);   // 1000:1 turns ratio, 200Ω burden resistor
/* or */
CT_Sensor mySensor(0, 20, 1);       // 1 volt output at 20 amps (built-in burden resistor)
```

### CT_Control(ctFreq_t freq)
##### Description
Defines a CT_Control object. Only one CT_Control object needs to be defined.
##### Syntax
`CT_Control ctCtrl(freq);`
##### Optional parameter
**freq:** AC line frequency, either `CT_FREQ_50HZ` or `CT_FREQ_60HZ`. Defaults to `CT_FREQ_60HZ` if not given. *(ctFreq_t)*  
##### Example
```c++
CT_Control myCtrl(CT_FREQ_50HZ);
```

## Library Functions

### float CT_Sensor::amps()
##### Description
Returns the current value in RMS amperes read by the last call to `CT_Control::read()`.
##### Syntax
`mySensor.amps();`
##### Parameters
None.
##### Returns
RMS current value in amperes *(float)*
##### Example
```c++
float rmsCurrent;
rmsCurrent = mySensor.amps();

```

### float CT_Control::begin()
##### Description
Initializes the AVR timer and ADC. (Timer/Counter1 is used to trigger the ADC conversions and so is not available for other purposes.)
##### Syntax
`myCtrl.begin();`
##### Parameters
None.
##### Returns
Microcontroller supply voltage in volts. *(float)*  
##### Example
```c++
float vcc = myCtrl.begin();
```

### void CT_Control::end()
##### Description
Resets the AVR timer and ADC to default configuration.
##### Syntax
`myCtrl.end();`
##### Parameters
None.
##### Returns
None.  
##### Example
```c++
myCtrl.end();
```

### void CT_Control::read(CT_Sensor *sensor1)
### void CT_Control::read(CT_Sensor *sensor1, CT_Sensor *sensor2)
##### Description
Given one or two CT_Sensor object addresses, measures the RMS current value for one AC cycle. After calling `read()`, access the measured RMS current value(s) via the `CT_Sensor::amps()` function.
##### Syntax
`myCtrl.read(&mySensor);`
##### Parameters
None.
##### Returns
None.
##### Example
```c++
CT_Sensor mySensor(0, 1000, 200);
CT_Control myCtrl(CT_FREQ_50HZ);
myCtrl.read(&mySensor);
float rmsCurrent = myCT.read();

```
