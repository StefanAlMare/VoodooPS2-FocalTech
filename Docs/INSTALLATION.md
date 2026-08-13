# Installation

## Required kexts

A release/build contains two top-level kexts:

```text
VoodooPS2Controller.kext
VoodooPS2FocalTech.kext
```

Inside `VoodooPS2Controller.kext/Contents/PlugIns/` this fork expects:

```text
VoodooPS2Keyboard.kext
VoodooInput.kext
```

The standard VoodooPS2 mouse and trackpad plugins are deliberately omitted from the final FocalTech package to avoid competing for the same PS/2 device.

## Validated macOS versions

The current **stable FLT stack** has been hardware-tested successfully on **macOS Sequoia** and **macOS Tahoe** on the validated FLT0101 and FLT0102 systems.

`FTE0001` support is currently experimental and is not yet included in the hardware-validation claim above.

## Important: OC Auxiliary Tools (OCAT)

> [!WARNING]
> On a machine using this FocalTech fork, if **OC Auxiliary Tools (OCAT)** reports an update for `VoodooPS2Controller.kext`, **do not let OCAT replace this fork's VoodooPS2 stack with the stock Acidanthera build** unless you intentionally want to remove FocalTech support.
>
> This fork uses a patched `VoodooPS2Controller.kext` and a separate `VoodooPS2FocalTech.kext`. The patched controller can have a **different file size and hash/MD5 from Acidanthera's stock binary even when the version number is identical**. OCAT may therefore show the stock package as an available update. **This is expected and is not an error.**
>
> Replacing the fork's VoodooPS2Controller/VoodooInput/FocalTech stack with the stock Acidanthera package can remove the FocalTech compatibility path and make the trackpad stop working.

For this stack, install updates from the **Releases page of this repository**. Other unrelated OCAT updates can still be handled normally.

## OpenCore Kernel -> Add order

```text
1. VoodooPS2Controller.kext
2. VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext
3. VoodooPS2FocalTech.kext
4. VoodooPS2Controller.kext/Contents/PlugIns/VoodooInput.kext
```

## FLT0101

Validated on an i8042 controller with active multiplexing and four AUX nubs. The FocalTech device attached on port 4 during testing.

**Do not add `foclegacy=1` or `focfte=1`.**

The driver probes mux ports safely and only attaches to the AUX nub that answers the FLT FocalTech signature.

## FLT0102

Validated on a simple AUX layout that requires the historical controller startup state before FocalTech native initialization.

Add this OpenCore boot argument:

```text
foclegacy=1
```

Do **not** add `focfte=1`.

The current implementation has been hardware-validated with `foclegacy=1` on FLT0102. Without it, some FLT0102 systems may create an incomplete PS/2 IORegistry tree and the trackpad may not attach.

## FLT0103

Protocol support is expected from the same six-byte FLT FocalTech family but has not yet been validated on physical FLT0103 hardware.

Start **without** `foclegacy=1` and **without** `focfte=1`. If the device does not attach, collect diagnostics before trying compatibility changes.

## FTE0001 — experimental

`FTE0001` is a **different FocalTech PS/2 protocol family** from FLT0101/0102/0103. It uses a separate `ApplePS2FTE0001` backend inside the same `VoodooPS2FocalTech.kext`.

FTE0001 probing is disabled by default. For experimental testing add:

```text
focfte=1
```

Do **not** use `focfte=1` on FLT0101 or FLT0102.

The initial implementation deliberately refuses muxed multi-AUX controller layouts and targets the simple AUX topology historically associated with FTE0001. While unvalidated, use a current **DEBUG artifact** rather than treating FTE0001 as part of the stable release claim.

See [`FTE0001.md`](FTE0001.md) for protocol details, limitations and the exact diagnostic commands.

## Driver conflicts

Do not simultaneously enable old `VoodooPS2FocalTech.kext`, ApplePS2SmartTouchPad, stock `VoodooPS2Trackpad.kext`, stock `VoodooPS2Mouse.kext`, or another PS/2 trackpad driver that can claim the same `ApplePS2MouseDevice`.

## Diagnostics

Useful checks for FLT:

```bash
kmutil showloaded | grep -Ei 'Voodoo|PS2|FocalTech'
ioreg -r -c ApplePS2FocalTech -lw0
ioreg -r -c ApplePS2MouseDevice -lw0
ioreg -p IOService -w0 | grep -Ei -C 3 'ApplePS2|VoodooPS2|FocalTech|VoodooInput'
```

For FTE0001:

```bash
ioreg -r -c ApplePS2FTE0001 -lw0
ioreg -p IOService -w0 | grep -Ei -C 3 'ApplePS2|FTE0001|FocalTech|VoodooInput'
```

The diagnostic build exposes counters including IRQ bytes, packet counts and valid/invalid FocalTech packets.
