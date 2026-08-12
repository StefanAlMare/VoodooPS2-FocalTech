#!/usr/bin/env python3
"""Apply the minimal FLT0102 controller compatibility patch.

Default controller behaviour stays identical to upstream.  The compatibility
path is enabled only when the user passes the boot argument foclegacy=1.

If upstream changes the two expected blocks, this script exits instead of
silently producing an unreviewed binary.
"""
from pathlib import Path
import sys

p = Path("VoodooPS2Controller/VoodooPS2Controller.cpp")
if not p.exists():
    sys.exit("ERROR: run from the VoodooPS2-FocalTech repository root")

s = p.read_text()
marker = "FocalTech controller compatibility v1"
if marker in s:
    print("Controller compatibility patch is already present.")
    sys.exit(0)

old1 = '''  PE_parse_boot_argn("ps2rst", &_resetControllerFlag, sizeof(_resetControllerFlag));
  if (_resetControllerFlag & RESET_CONTROLLER_ON_BOOT) {
    resetController();
  }

  //
'''

new1 = '''  PE_parse_boot_argn("ps2rst", &_resetControllerFlag, sizeof(_resetControllerFlag));

  // FocalTech controller compatibility v1.
  // Modified by StefanAlMare, 2026-08-13.
  // Default: upstream behaviour.
  // foclegacy=1: startup sequence validated on FLT0102 hardware.
  int focalTechLegacyInit = 0;
  PE_parse_boot_argn("foclegacy", &focalTechLegacyInit, sizeof(focalTechLegacyInit));

  if (focalTechLegacyInit) {
    writeCommandPort(kCP_GetCommandByte);
    UInt8 commandByte = readDataPort(kPS2KbdIdx);
    commandByte &= ~(kCB_EnableMouseIRQ | kCB_DisableMouseClock);
    writeCommandPort(kCP_SetCommandByte);
    writeDataPort(commandByte);

    writeDataPort(kDP_SetDefaultsAndDisable);
    readDataPort(kPS2KbdIdx);

    if (!_kbdOnly) {
      writeCommandPort(kCP_TransmitToMouse);
      writeDataPort(kDP_SetDefaultsAndDisable);
      readDataPort(kPS2AuxIdx);
    }

    flushDataPort();

    writeDataPort(kDP_Reset);
    readDataPort(kPS2KbdIdx);
  }
  else if (_resetControllerFlag & RESET_CONTROLLER_ON_BOOT) {
    resetController();
  }

  //
'''

old2 = '''  if (_resetControllerFlag & RESET_CONTROLLER_ON_BOOT) {
    resetDevices();
    flushDataPort();
  }
'''

new2 = '''  if (focalTechLegacyInit || (_resetControllerFlag & RESET_CONTROLLER_ON_BOOT)) {
    if (!focalTechLegacyInit) {
      resetDevices();
    }
    flushDataPort();
  }
'''

if s.count(old1) != 1:
    sys.exit("ERROR: upstream changed the ps2rst/resetController block; patch review required")
s = s.replace(old1, new1, 1)

if s.count(old2) != 1:
    sys.exit("ERROR: upstream changed the post-mux resetDevices block; patch review required")
s = s.replace(old2, new2, 1)

p.write_text(s)
print("Applied FocalTech controller compatibility v1.")
