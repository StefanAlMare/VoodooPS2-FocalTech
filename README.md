# VoodooPS2-FocalTech

A maintained fork of [Acidanthera/VoodooPS2](https://github.com/acidanthera/VoodooPS2) adding a standalone **FocalTech PS/2 multitouch trackpad driver** for older laptops whose FocalTech touchpads are not handled correctly by the standard VoodooPS2 trackpad stack.

The FocalTech driver reports through **VoodooInput / native macOS multitouch**, instead of implementing macOS gestures itself.

## Status

| Hardware ID | Status | Controller layout | Notes |
|---|---|---|---|
| **FLT0101** | ✅ Confirmed | i8042 active multiplexing / 4 AUX nubs | Mux-safe path; no legacy boot argument |
| **FLT0102** | ✅ Confirmed on legacy-compatible controller path | simple i8042 AUX | use `foclegacy=1` |
| **FLT0103** | 🧪 Experimental | FocalTech PS/2 family | protocol support expected, not hardware-validated yet |

### Confirmed functionality

- native cursor movement
- multitouch reporting (up to 5 contacts)
- macOS Trackpad preference pane
- scrolling and macOS gestures through VoodooInput
- physical left click
- software-defined bottom-right secondary click
- dragging
- progressive Force Click / Look Up emulation after a stationary physical left-button hold
- sleep/wake reinitialization path
- safe handling of muxed i8042 controllers

## Architecture

```text
VoodooPS2Controller
        │
        ├── VoodooPS2Keyboard
        │
        └── ApplePS2MouseDevice
                 │
                 └── VoodooPS2FocalTech
                          │
                          └── VoodooInput
                                   │
                                   └── macOS native multitouch
```

`VoodooPS2FocalTech.kext` is a separate client driver. It still intentionally depends on VoodooPS2Controller for PS/2 transport and on VoodooInput for multitouch reporting.

## Build

Requirements: macOS, full Xcode, Git and Internet access.

```bash
chmod +x build_focaltech.sh
./build_focaltech.sh
```

The build is done in an isolated temporary copy and produces:

```text
~/Desktop/VoodooPS2-FocalTech-Build/Kexts/
├── VoodooPS2Controller.kext
│   └── Contents/PlugIns/
│       ├── VoodooPS2Keyboard.kext
│       └── VoodooInput.kext
└── VoodooPS2FocalTech.kext
```

The script applies the FLT0102 controller compatibility patch only in the temporary build tree. If upstream changes the expected controller startup blocks, the patch **fails closed** and asks for review instead of producing an unreviewed binary.

## OpenCore order

Under `Kernel -> Add`:

1. `VoodooPS2Controller.kext`
2. `VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext`
3. `VoodooPS2FocalTech.kext`
4. `VoodooPS2Controller.kext/Contents/PlugIns/VoodooInput.kext`

See [Docs/INSTALLATION.md](Docs/INSTALLATION.md) for hardware-specific settings.

## Upstream synchronization

This repository is a fork of Acidanthera/VoodooPS2. Use GitHub's **Sync fork** function or merge the newest upstream `master` before building. The FocalTech controller patch is deliberately small and guarded so upstream changes are easy to review.

## Credits and provenance

This project exists because of work done by many people over many years. In particular:

- **Acidanthera and all VoodooPS2 contributors** for the modern VoodooPS2 controller and trackpad framework.
- **Apple** and the historical Apple PS/2 source on which the family of PS/2 drivers ultimately builds.
- **VoodooInput / VoodooI2C contributors** for Magic Trackpad 2 emulation and the native multitouch reporting path.
- **EMlyDinEsH**, author of **ApplePS2SmartTouchPad** and its historical ELAN/FocalTech/Synaptics work. ApplePS2SmartTouchPad was an essential practical reference while recovering FocalTech behaviour on hardware that modern VoodooPS2 did not handle.
- **Linux kernel input developers**, particularly the contributors to the FocalTech PS/2 driver, for publicly documented packet formats, device IDs and native protocol behaviour.
- The earlier **VoodooPS2 / ApplePS2 / RehabMan-era contributors** credited by upstream.
- **StefanAlMare** for the modern standalone FocalTech integration, controller compatibility work, mux-safe probing, native VoodooInput reporting, click-zone handling and progressive Force Click emulation in this fork.

See [CREDITS.md](CREDITS.md) for a fuller attribution statement.

## License

The upstream repository is distributed under the **Apple Public Source License 2.0**. The original notices and `LICENSE.md` are retained. New FocalTech source in this fork is also distributed under APSL 2.0 unless a file explicitly states otherwise.

This is an independent community fork and is **not an official Acidanthera project**.
