# How Pumpunjuksautin works

Pulls voltage over NTC thermistor while recovering original thermistor
resistance.

The firmware has bookkeeping about the duty cycle. For example if the
*DRIVE* is in LOW state for 10 ms and in HI-Z state for 90 ms, the duty
cycle D is 0.9.

Fill in parameters in this formula from the circuit below:

<img src="https://render.githubusercontent.com/render/math?math=\LARGE R_t=\left(\frac{\frac{U_m}{U_f}-1}{R_m}-\frac{D}{R_d}\right)^{-1}">

![Variables in the formula](variables.svg)

This allows us to fake a higher temperature reading and at the same time
do measurements. All this without disconnecting the thermistor from the
circuit.

## Is it possible to fake lower temperatures, too?

Yes, but the formulas aren't derived yet... We can put *DRIVE* to 5
volts and observe the duty cycle.
