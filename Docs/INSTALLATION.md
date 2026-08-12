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

The current stack has been hardware-tested successfully on **macOS Sequoia** and **macOS Tahoe** on the validated FLT0101 and FLT0102 systems.

## Important: OpenCore Legacy Patcher (OCLP)

> [!WARNING]
> On a machine using this FocalTech fork, if OCLP offers to update or replace the VoodooPS2-related kexts, **do not accept that VoodooPS2 replacement** unless you intentionally want to remove this fork.
>
> `VoodooPS2FocalTech.kext` intentionally has its own bundle identifier and this package also contains FocalTech-specific VoodooPS2Controller behaviour. OCLP can therefore see its stock VoodooPS2 package as different and offer its own version. **This is expected and is not an error.**
>
> Replacing the fork's VoodooPS2Controller/VoodooInput/FocalTech stack with the stock OCLP package can remove FocalTech support and make the trackpad stop working.

For this stack, install updates from the **Releases page of this repository**. Other unrelated OCLP updates can still be handled normally.

## OpenCore Kernel -> Add order

```text
1. VoodooPS2Controller.kext
2. VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext
3. VoodooPS2FocalTech.kext
4. VoodooPS2Controller.kext/Contents/PlugIns/VoodooInput.kext
```

## FLT0101

Validated on an i8042 controller with active multiplexing and four AUX nubs. The FocalTech device attached on port 4 during testing.

**Do not add `foclegacy=1`.**

The driver probes mux ports safely and only attaches to the AUX nub that answers the FocalTech signature.

## FLT0102

Validated on a simple AUX layout that requires the historical controller startup state before FocalTech native initialization.

Add this OpenCore boot argument:

```text
foclegacy=1
```

The current implementation has been hardware-validated with this boot argument on FLT0102. Without it, some FLT0102 systems may create an incomplete PS/2 IORegistry tree and the trackpad may not attach.

## FLT0103

Protocol support is expected from the same FocalTech family but has not yet been validated on physical FLT0103 hardware.

Start **without** `foclegacy=1`. If the device does not attach, collect diagnostics before trying compatibility changes.

## Diagnostics

Useful checks:

```bash
kmutil showloaded | grep -Ei 'Voodoo|PS2|FocalTech'
ioreg -r -c ApplePS2FocalTech -lw0
ioreg -r -c ApplePS2MouseDevice -lw0
ioreg -p IOService -w0 | grep -Ei -C 3 'ApplePS2|VoodooPS2|FocalTech|VoodooInput'
```

The diagnostic build exposes counters including IRQ bytes, packet counts and valid/invalid FocalTech packets.
