# JuksUtil

Utility for communicating with JuksOS devices, including
Pumpunjuksautin. Supports both ASCII and Modbus protocols.

## Compiling

Requires a C compiler, libmodbus and glib libraries and CMake to build. In Debian
and Ubuntu, you can install the requirements with:

```sh
sudo apt install cmake libmodbus-dev libglib2.0-dev
```

To build:

```sh
mkdir build
cd build
cmake ..
make
```

## Summary of commands

In principle, if when using ASCII interface, you must supply device
path with `-d`. With Modbus, you additionally need server id (better
known as slave id) with `-s`.

Time zone related commands such as `show-transition` and `sync-time`
accept `-z` parameter for supplying the desired time zone.

Help is available with `juksutil --help`.

### show-transition

Show current time zone and future DST transition, if any. Doesn't need
any device. It just shows what would be written. Also, it's
suprisingly useful tool to check DST changes overall, in case you are
interested about time zone stuff like me.

<pre>
$ juksutil show-transition
Current and following time zone for <strong>Europe/Berlin</strong>:

<strong>                  UTC time              Local time                UNIX time   UTC off</strong>
<strong>Current time:</strong>     2021-10-03T17:03:07Z  2021-10-03T19:03:07+0200  1633280587    +7200
<strong>Next transition:</strong>  2021-10-31T01:00:00Z  2021-10-31T02:00:00+0100  1635642000    +3600
</pre>

### get-time

Get current time from the device. Format is ISO 8601 timestamp with
time zone. It shows the current time and time zone the device is
observing.

### sync-time

Synchronize clock of JuksOS device. Sets also DST transition
table. Supply the desired time zone with `-z` or otherwise your
computer zone is used.

The command outputs nothing on success. To verify it, you may run
`juksutil get-time`.

### send

Read and/or write values from/to the hardware via ASCII
interface. This requires that the device firmware is built with ASCII
interface (CMake variable `-D WITH_ASCII=ON`).

The format is: `send KEY[=VALUE]..`, example: `juksutil send version led=1`.

The supported commands are described in [commands.tsv](../avr/commands.tsv).

## How about supporting the remaining Modbus commands?

Not all Modbus commands are supported because there is better tools for
doing the stuff which are not timing critical like setting clock
time. One of them is a nice command-line utility called
[modbus-cli](https://github.com/favalex/modbus-cli).

Modbus register chart is available in [commands.tsv](../avr/commands.tsv).
