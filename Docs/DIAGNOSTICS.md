# Diagnostics and hardware reports

When reporting a FocalTech PS/2 problem, please collect data before changing multiple drivers or controller settings. The goal is to determine whether the failure is device detection, controller routing, packet framing, or VoodooInput reporting.

## macOS

### Loaded kexts

```bash
kmutil showloaded | grep -Ei 'Voodoo|PS2|FocalTech'
```

### FLT service — FLT0101 / FLT0102 / FLT0103

```bash
ioreg -r -c ApplePS2FocalTech -lw0
```

Important FLT properties include:

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

### FTE0001 service — experimental

FTE0001 requires `focfte=1` and uses a separate service class:

```bash
ioreg -r -c ApplePS2FTE0001 -lw0
```

Important FTE0001 properties include:

```text
FocalTechProtocol = FTE0001
FTE0001Experimental
FTE0001OptIn
FTE0001Product0
FTE0001Product1
FTE0001Product2
FTE0001FingerCount
FocalTechPort
FocalTechAuxNubs
FocalTechIRQBytes
FocalTechPackets
FocalTechValidPackets
FocalTechInvalidPackets
FocalTechLastPacketLength
FocalTechLast0 ... FocalTechLast15
```

A detected FTE0001 device should expose product bytes `58 00 05`.

### PS/2 providers and tree

```bash
ioreg -r -c ApplePS2MouseDevice -lw0
ioreg -p IOService -w0 | grep -Ei -C 4 'ApplePS2|VoodooPS2|FTE0001|FocalTech|VoodooInput'
```

### Boot arguments

```bash
nvram -p 2>/dev/null | grep -i boot-args
```

Relevant project boot arguments:

```text
FLT0101: no foclegacy=1, no focfte=1
FLT0102: foclegacy=1, no focfte=1
FLT0103: experimental; normally neither argument
FTE0001: focfte=1; experimental
```

## Interpreting counters

- `FocalTechIRQBytes = 0`: the client is attached but no interrupt data reaches it; investigate controller/port routing or device streaming.
- IRQ bytes increase but `FocalTechValidPackets = 0`: the controller delivers data but packet framing/native protocol is wrong.
- `FocalTechValidPackets` increases and invalid packets stay at zero: the FocalTech transport/parser is working; investigate VoodooInput/reporting above it.
- On the FLT six-byte native stream, `FocalTechIRQBytes` should normally be six times the complete packet count.
- FTE0001 uses normalized 16-byte ring-buffer records: physical reports are 8 bytes for up to two contacts and 16 bytes for three/four contacts. The short reports are padded internally for safe workloop processing.

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

Also include the ACPI hardware ID (`FLT0101`, `FLT0102`, `FLT0103`, `FTE0001` or another ID), laptop model, BIOS version, and whether i8042 multiplexing is active.

## Keep tests controlled

Change one variable at a time. In particular, do not simultaneously replace the controller, FocalTech kext, VoodooInput, ACPI tables, and boot arguments; doing so makes a useful comparison impossible.

For FTE0001 testing, keep a known-good EFI available and use a current DEBUG artifact until physical hardware validation is completed.
