# Contributing

Hardware reports, protocol findings and code improvements are welcome.

## Before opening an issue

1. Confirm the touchpad is actually a FocalTech PS/2 device.
2. Read `Docs/INSTALLATION.md` and `Docs/DIAGNOSTICS.md`.
3. Test with the current `master` build and record the exact commit SHA.
4. Collect macOS IORegistry and, when possible, Linux i8042/serio identification.

## Code changes

Keep changes narrowly scoped and explain which hardware path they affect. Controller changes are particularly sensitive because i8042 state is shared by keyboard and AUX devices.

Any change to the FLT0102 compatibility startup should preserve the default upstream controller behaviour when `foclegacy=1` is absent.

Any change to mux probing must avoid sending global controller/keyboard compatibility commands while probing empty AUX mux ports.

## Attribution

Preserve upstream license headers and contributor notices. If code or behaviour is derived from another open-source implementation, identify that source and its license in the commit/PR description and in source comments where appropriate.
