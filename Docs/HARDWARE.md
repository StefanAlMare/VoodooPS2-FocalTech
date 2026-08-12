# FocalTech hardware notes

## Supported family

The driver targets FocalTech PS/2 devices identified by the Linux driver family as:

- `FLT0101`
- `FLT0102`
- `FLT0103`

## Protocol

Native FocalTech traffic uses six-byte packets with up to five contacts.

Known packet types:

```text
0x3  TOUCH
0x6  ABS
0x9  REL
```

The driver validates finger indices before accessing the five-finger state array and reports coordinates through VoodooInput.

## Tested FLT0101 layout

```text
ACPI: FLT0101
controller: i8042 active multiplexing
AUX nubs: 4
working FocalTech provider port: 4
legacy fallback: disabled
```

A validated run received 9,408 IRQ bytes = 1,568 complete six-byte packets, all 1,568 accepted and zero rejected packets.

## Tested FLT0102 layout

```text
ACPI: FLT0102
controller: standard/simple i8042 AUX
legacy controller startup: required on tested hardware
boot argument: foclegacy=1
```

## Clickpad behaviour

The tested FocalTech hardware exposes one physical click switch. Therefore distinct left/right hardware buttons do not exist. The driver maps:

- normal physical click -> left click
- physical click in bottom 30% + right half -> right click

The selected side is latched until release, which preserves drag behaviour.

## Force Click emulation

The hardware has no true pressure sensor usable as a modern Force Touch signal. This fork provides progressive emulation:

- physical left click is immediate
- if the click remains essentially stationary for about two seconds, the driver releases the direct mouse-button channel and emits a synthetic pressure frame
- movement beyond the configured threshold cancels promotion, preserving normal drag
