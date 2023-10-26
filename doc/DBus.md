# TuxClocker DBus API documentation
## Overview
TuxClocker uses a DBus daemon to expose device functionality. There are a couple reasons for this:

- It allows other programs than just the TuxClocker Qt GUI to use the functionality, allowing other programs and scripts to use it without the GUI running
- Setting most properties require elevated permissions, allowing normal users access to the functionality

## DBus object structure
The registered paths in the TuxClocker DBus services look something like this:
```
/9a60781a452ed4abb58ecdc5688a41fc
/9a60781a452ed4abb58ecdc5688a41fc/51214b57ddb9b6b4aabe7d0cbb309e34
/9a60781a452ed4abb58ecdc5688a41fc/8a296374e7884780ed9376945ef144a7
```
Each object's path is its hash, which is intended to be a locale independent and as accurate as possible way to identify the device and interface nodes uniquely. The names of the nodes could be something like:

```
NVIDIA GeForce 1060
NVIDIA GeForce 1060/Fan Speed
NVIDIA GeForce 1060/Core Clock
```

## DBus object interfaces
Note: the types are expressed as DBus types.
### org.tuxclocker.Node
All nodes except `/` implement org.tuxclocker.Node.
#### Properties
`s name`: The node's intended display text, eg. "Fan Speed".

`s hash`: The hash to uniquely identify the node. The same as the node's "file name". Eg. for `/ad4h/y92w` `hash` would be `y92w`.
### org.tuxclocker.DynamicReadable
Represents a readable property that may change, eg. GPU temperature.
#### Properties
`(bs) unit`: `b`: if unit is missing. `s`: unit of the value, eg. '%'.
#### Methods
`() -> (bv) value`: `b`: if there was an error fetching the value. `v` represents `i | u | d`; the current value.

### org.tuxclocker.StaticReadable
Represents a static value such as slowdown temperature.
#### Properties
`(bs) unit`: `b`: if unit is missing. `s`: unit of the value, eg. '%'.

`v value`: `v` represents `i | u | d`; the value.

### org.tuxclocker.Assignable
Represents a writable property such as power limit.
#### Properties
`(bs) unit`: `b`: if unit is missing. `s`: unit of the assignable, eg. '%'.

`v assignableInfo`: information about the assignable's possible values. `v` represents `(vv) | a(us)`, where:

`(vv)` is the inclusive range of valid values, where both `v` represent `i | d`.

`a(us)` represents a list of discrete settings the assignable can be set to. `u` represents the key of the setting that is used when calling `assign`. `s` is the text representation of the setting, eg. 'Manual' as in manual fan speed.
#### Methods
`(v) -> (bi) assign`: attempts to set a new value for the assignable. `v` represents `i | u | d` matching the type from `assignableInfo`. `b`: if there was an error. `i`: TuxClocker::AssignmentError integer representation if `b` is true.

`() -> (bv) currentValue`: `b`: if current value couldn't be fetched. `v` represents `i | u | d` matching the type from `assignableInfo`.
