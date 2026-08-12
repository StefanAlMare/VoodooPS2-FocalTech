# Credits and provenance

VoodooPS2-FocalTech is a derivative/community project. Attribution is important because the working driver is the result of several independent bodies of prior work.

## Upstream VoodooPS2

Primary upstream project: [Acidanthera/VoodooPS2](https://github.com/acidanthera/VoodooPS2).

All authors and contributors already credited by upstream remain credited here. This fork does not replace or diminish those notices.

## Apple

The VoodooPS2 lineage contains and derives from publicly released Apple PS/2 source. Original Apple copyright and license notices remain in the upstream source files where applicable.

## EMlyDinEsH / ApplePS2SmartTouchPad

**EMlyDinEsH** authored `ApplePS2SmartTouchPad`, historically published as the **ELAN, FocalTech and Synaptics (Smart Touchpad) Driver** for OS X.

That driver was particularly important to this project because it was known to operate the FocalTech hardware used during development when modern VoodooPS2 did not. Its observed initialization behaviour and FocalTech support provided an essential historical/practical reference during reverse engineering and compatibility work.

Historical discussion/reference:
- https://osxlatitude.com/forums/topic/1948-elan-focaltech-and-synaptics-smart-touchpad-driver/

Credit here is for prior authorship and technical influence/reference. This project does not claim authorship of EMlyDinEsH's work.

## Linux kernel FocalTech driver

The Linux input subsystem's FocalTech PS/2 implementation provided an open reference for:

- FLT0101 / FLT0102 / FLT0103 identification
- six-byte native packet layout
- touch / absolute / relative packet types
- coordinate extraction
- register access and native protocol switching
- FocalTech device dimensions and Y-axis handling

Linux kernel contributors retain credit for that work under the Linux kernel's own licensing terms.

## VoodooInput / VoodooI2C

VoodooInput and the VoodooI2C ecosystem provided the native multitouch architecture and Magic Trackpad 2 emulation path used by this driver. Their upstream contributor lists remain authoritative.

## StefanAlMare

Work specific to this fork includes:

- standalone `ApplePS2FocalTech` client design for modern VoodooPS2
- FLT0101 mux-safe probing and AUX-port handling
- FLT0102 legacy-compatible controller startup mode (`foclegacy=1`)
- native VoodooInput transducer reporting
- FocalTech packet parser hardening
- software left/right click zoning for single-switch clickpads
- drag preservation
- progressive timer-based Force Click / Look Up emulation
- diagnostics used to validate IRQ bytes and native packet framing on real hardware

## Tested hardware contributors

Additional hardware reports and contributions are welcome. When a new FocalTech model is validated, contributors should be credited in the relevant release notes and hardware table.
