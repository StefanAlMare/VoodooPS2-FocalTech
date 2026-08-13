# Supported hardware and limitations

VoodooPS2-FocalTech is a specialised VoodooPS2 fork for **FocalTech PS/2 trackpads**. It is not intended to replace the complete upstream VoodooPS2 trackpad family for unrelated hardware.

## Hardware status

| Hardware ID | Status | Notes |
|---|---|---|
| **FLT0101** | ✅ Hardware validated | i8042 mux-safe path; tested with four AUX nubs; do **not** use `foclegacy=1` |
| **FLT0102** | ✅ Hardware validated | simple i8042 AUX path; requires `foclegacy=1` on the validated system |
| **FLT0103** | 🧪 Experimental | protocol-family support is implemented, but no physical FLT0103 device has yet been validated |
| Other FocalTech PS/2 IDs | 🧪 Unsupported / experimental | may be tried for testing, but compatibility is not claimed |

The validated FLT0101 and FLT0102 systems have been tested successfully on **macOS Sequoia** and **macOS Tahoe**.

## What this package is for

Use this project when the machine has a **FocalTech PS/2 trackpad** that is not handled correctly by the standard VoodooPS2 trackpad stack.

The package provides:

- patched `VoodooPS2Controller.kext` with the optional FLT0102 compatibility path;
- `VoodooPS2Keyboard.kext`;
- pinned `VoodooInput.kext`;
- standalone `VoodooPS2FocalTech.kext`;
- native macOS multitouch reporting through VoodooInput.

## What this package is not for

This fork is **not targeted at unrelated PS/2 touchpad families**, including:

- Synaptics;
- ELAN / Elantech devices that are not FocalTech;
- ALPS;
- Sentelic;
- other generic PS/2 mice or trackpads handled by the normal upstream VoodooPS2 plugins.

Those devices should normally use the appropriate upstream VoodooPS2 driver/plugin instead.

The final FocalTech package deliberately omits the stock VoodooPS2 mouse and trackpad plugins so they do not compete with `VoodooPS2FocalTech.kext` for the same PS/2 device.

## Trying unvalidated FocalTech hardware

An unvalidated FocalTech PS/2 device may be tested, but that is **experimental**. Do not interpret the presence of the FocalTech name alone as a compatibility guarantee.

For unknown hardware:

1. start without `foclegacy=1` unless the device specifically exhibits the FLT0102-style incomplete PS/2 tree;
2. use the DEBUG package when collecting diagnostics;
3. report the ACPI hardware ID, IORegistry tree and FocalTech diagnostic counters in an issue.

## Important OCAT note

OCA Auxiliary Tools (OCAT) may report the stock Acidanthera `VoodooPS2Controller.kext` as an available update because this fork's controller binary has a different file size and hash/MD5 even when the version number matches upstream. This difference is expected.

Do not let OCAT replace the FocalTech stack with stock VoodooPS2 unless you intentionally want to remove this fork. Use this repository's Releases for FocalTech updates.
