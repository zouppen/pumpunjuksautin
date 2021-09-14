# Pumpunjuksautin - Defrauding heat pumps for fun and profit

## Introduction
Pumpunjuksautin is a device for introducing a measure of load control into
heating systems where no external control features such as automation buses
exist. The core idea is to modify the signal the heating controller gets from
the thermistor measuring it's warm water accumulator temperature. By modifying
this signal so that the accumulator seems warmer than it is in actuality, the
heating can be impeded until the desired time.

In practice, Pumpunjuksautin is meant to be attached to Modbus RTU bus, through
which it is controlled by building automation. Typically the heating duty cycle
is delayed until either energy market or solar power generation allow for cost
efficient heating.

## Repository content
This repository contains the design of the whole Pumpunjuksautin device,
divided to it's components:
* [Firmware source code](avr/README.md)
* [Documentation](docs/)
* [Enclosure design](enclosure/README.md)
* [Printed circuit board design](pcb/)
* [Operational utilities](tools/)

## License
The contents of this repository are licensed under the GNU General Public
License version 3 or later. See file LICENSE for the license text.