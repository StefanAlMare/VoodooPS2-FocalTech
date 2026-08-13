# Contributing

Hardware reports, protocol findings and code improvements are welcome.

## Before opening an issue

1. Confirm the touchpad is actually a FocalTech PS/2 device.
2. Read `Docs/INSTALLATION.md` and `Docs/DIAGNOSTICS.md`.
3. For FTE0001, also read `Docs/FTE0001.md` and test only with `focfte=1`.
4. Test with the current `master` DEBUG build/artifact and record the exact commit SHA.
5. Collect macOS IORegistry and, when possible, Linux i8042/serio identification.

## Code changes

Keep changes narrowly scoped and explain which hardware path they affect. Controller changes are particularly sensitive because i8042 state is shared by keyboard and AUX devices.

Any change to the FLT0102 compatibility startup should preserve the default upstream controller behaviour when `foclegacy=1` is absent.

Any change to mux probing must avoid sending global controller/keyboard compatibility commands while probing empty AUX mux ports.

FTE0001 is a separate protocol backend. Changes to `ApplePS2FTE0001` must not alter normal FLT probing when `focfte=1` is absent. Until FTE0001 is validated on physical hardware, keep probing opt-in and avoid muxed multi-AUX controller layouts.

## Attribution

Preserve upstream license headers and contributor notices. If code or behaviour is derived from another open-source implementation, identify that source and its license in the commit/PR description and in source comments where appropriate.

The historical `chilledHamza/VoodooPS2FocalTech` repository is GPLv3 and is an important FTE0001 reference. Do not copy GPL source into this APSL fork; protocol work for FTE0001 should remain independently implemented and properly credited.
