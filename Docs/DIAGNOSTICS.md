# Diagnostics and hardware reports

When reporting a FocalTech PS/2 problem, please collect data before changing multiple drivers or controller settings. The goal is to determine whether the failure is device detection, controller routing, packet framing, or VoodooInput reporting.

## macOS

### Loaded kexts

```bash
kmutil showloaded | grep -Ei 'Voodoo|PS2|FocalTech'
```

### FocalTech service

```bash
ioreg -r -c ApplePS2FocalTech -lw0
```

Important properties include:

```text
FocalTechPort
FocalTechAuxNubs
FocalTechLegacyFallback
FocalTechIRQBytes
FocalTechPackets
FocalTechValidPackets
FocalTechInvalidPackets
FocalTechLastType
FocalTechLast0 ... FocalTechLast5
```

### PS/2 providers and tree

```bash
ioreg -r -c ApplePS2MouseDevice -lw0
ioreg -p IOService -w0 | grep -Ei -C 4 'ApplePS2|VoodooPS2|FocalTech|VoodooInput'
```

### Boot arguments

```bash
nvram -p 2>/dev/null | grep -i boot-args
```

## Interpreting counters

- `FocalTechIRQBytes = 0`: the client is attached but no interrupt data reaches it; investigate controller/port routing or device streaming.
- IRQ bytes increase but `FocalTechValidPackets = 0`: the controller delivers data but packet framing/native protocol is wrong.
- `FocalTechValidPackets` increases and invalid packets stay at zero: the FocalTech transport/parser is working; investigate VoodooInput/reporting above it.
- On a six-byte native stream, `FocalTechIRQBytes` should normally be six times the complete packet count.

## Linux / Ubuntu hardware identification

If Linux identifies the touchpad correctly, include:

```bash
cat /proc/bus/input/devices
```

and:

```bash
grep -R . /sys/bus/serio/devices/*/{description,firmware_id,modalias} 2>/dev/null
```

Useful kernel messages:

```bash
dmesg | grep -Ei 'focal|i8042|serio|ps/2'
```

Also include the ACPI hardware ID (`FLT0101`, `FLT0102`, `FLT0103` or another ID), laptop model, BIOS version, and whether i8042 multiplexing is active.

## Keep tests controlled

Change one variable at a time. In particular, do not simultaneously replace the controller, FocalTech kext, VoodooInput, ACPI tables, and boot arguments; doing so makes a useful comparison impossible.
