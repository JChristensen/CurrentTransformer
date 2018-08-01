# Arduino CurrentTransformer Library
https://github.com/JChristensen/CurrentTransformer  
README file  

## License
Arduino CurrentTransformer Library Copyright (C) 2018 Jack Christensen GNU GPL v3.0

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License v3.0 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/gpl.html>

## Introduction
The CurrentTransformer library measures RMS current values in a 50/60Hz AC circuit using current transformers. Each call to `read()` causes the analog-to-digital converter (ADC) to measure a single AC cycle. The data gathered is processed using the [standard RMS calculation](https://en.wikipedia.org/wiki/Root_mean_square#Definition) and the result in amperes is returned to the caller.

Since each call to `read()` measures only a single cycle, it may be desirable, depending on the application and the nature of the electrical load, to make several calls, perhaps averaging the results, removing transients, etc. Each call to `read()` only takes about as long as one AC cycle, so there is not a lot of overhead or delay in taking several measurements.

Because measuring AC causes the current transformer to output positive and negative currents, a DC bias must be applied to ensure that below-ground voltages are not applied to the microcontroller's ADC input. It is also necessary to ensure that peak voltages do not exceed the microcontroller's supply voltage (Vcc). Do the math to ensure that your circuit operates within safe limits; also see the example below.

This library is specific to the AVR microcontroller architecture and will not work on others. Timer/Counter1 is used to trigger the ADC conversions and so is not available for other purposes.

## Example Design Calculations

The TA17L-03 current transformer is rated at 10A maximum and has a 1000:1 turns ratio. A 200Ω burden resistor is recommended. A 10A RMS current in the primary will generate a 10mA current in the secondary and therefore 2V across the burden resistor. However the peak voltage will then be √2 * 2V = ±2.8V (assuming a sine wave) which exceeds the 2.5V DC bias provided by the circuit below. Therefore the measured current should be limited to about 8.5A RMS (giving ±2.4V P-P) or perhaps a smaller burden resistor could be used if larger currents need to be measured.

## Typical Circuit
![](https://raw.githubusercontent.com/JChristensen/CurrentTransformer/master/typical-circuit.png)

## Enumeration
### ctFreq_t
##### Description
Operating frequency for the current transformer.
##### Values
- CT_FREQ_50HZ
- CT_FREQ_60HZ

## Constructor

### CurrentTransformer(uint8_t channel, float ratio, float burden, float vcc, ctFreq_t freq)
##### Description
The constructor defines a CurrentTransformer object.
##### Syntax
`CurrentTransformer(channel, ratio, burden, vcc, freq);`
##### Required parameters
**channel:** ADC channel number that the current transformer is connected to. (Arduino pin numbers can also be used, i.e. A0-A5). *(uint8_t)*  
**ratio:** Secondary:Primary turns ratio for the current transformer. *(float)*
**burden:** Current transformer burden resistor value in ohms. *(float)*
##### Optional parameters
**vcc:** Microcontroller supply voltage. For best accuracy, measure the actual microcontroller supply voltage and provide it using this parameter. Defaults to 5.0V if not given. *(float)*  
**freq:** AC line frequency, either CT_FREQ_50HZ or CT_FREQ_60HZ. Defaults to CT_FREQ_60HZ if not given. *(ctFreq_t)*  
##### Example
```c++
CurrentTransformer myCT(0, 1000, 200, 5.08, CT_FREQ_50HZ);
```
## Library Functions

### void begin()
##### Description
Initializes the AVR timer and ADC. If more than one CurrentTransformer object is defined, `begin()` only needs to be called for one of the objects, although calling it on more than one object will not cause an issue.
##### Syntax
`myCT.begin();`
##### Parameters
None.
##### Returns
None.
##### Example
```c++
myCT.begin();
```
### float read()
##### Description
Measures the RMS current value for one AC cycle and returns the value in amperes.
##### Syntax
`myCT.read();`
##### Parameters
None.
##### Returns
RMS current value in amperes *(float)*
##### Example
```c++
float rmsCurrent;
rmsCurrent = myCT.read();

```
