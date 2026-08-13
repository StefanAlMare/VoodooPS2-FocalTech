# VoodooPS2-FocalTech

A maintained fork of [Acidanthera/VoodooPS2](https://github.com/acidanthera/VoodooPS2) adding a standalone **FocalTech PS/2 multitouch trackpad driver** for older laptops whose FocalTech touchpads are not handled correctly by the standard VoodooPS2 trackpad stack.

The FocalTech driver reports through **VoodooInput / native macOS multitouch**, instead of implementing macOS gestures itself.

## Downloads

For most users, download a precompiled package from **[Releases](https://github.com/StefanAlMare/VoodooPS2-FocalTech/releases)**. You do **not** need Xcode or source compilation to install the Release package.

- **`VoodooPS2-FocalTech-<version>-RELEASE.zip`** — ready-to-use package, recommended for normal use.
- **`VoodooPS2-FocalTech-<version>-DEBUG.zip`** — development/diagnostics package for contributors, hardware bring-up and bug reports. Use this when collecting diagnostics for unvalidated hardware; it is not recommended for normal daily use when RELEASE works correctly.

Both packages include installation instructions, credits, source/build metadata and **`SUPPORTED-HARDWARE.md`**, which states the supported hardware and limitations. See [Docs/SUPPORTED-HARDWARE.md](Docs/SUPPORTED-HARDWARE.md) before trying the driver on unvalidated hardware.

## Status

| Hardware ID | Status | Controller layout | Notes |
|---|---|---|---|
| **FLT0101** | ✅ Hardware validated | i8042 active multiplexing / 4 AUX nubs | Mux-safe path; no legacy boot argument |
| **FLT0102** | ✅ Hardware validated | simple i8042 AUX | use `foclegacy=1` |
| **FLT0103** | 🧪 Experimental | FocalTech PS/2 family | protocol support expected, not hardware-validated yet |

Other/unknown FocalTech PS/2 devices may be tried for testing, but compatibility is **experimental until physically validated**. This fork is not intended for unrelated Synaptics, non-FocalTech ELAN/Elantech, ALPS, Sentelic or generic PS/2 touchpad/mouse hardware.

### macOS validation

The current FocalTech stack has been hardware-tested successfully on **macOS Sequoia** and **macOS Tahoe** on the validated FLT0101 and FLT0102 systems.

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

## Important note for OC Auxiliary Tools (OCAT) users

> [!WARNING]
> If **OC Auxiliary Tools (OCAT)** reports an update for `VoodooPS2Controller.kext` on a machine using this fork, **do not let OCAT replace this fork's VoodooPS2 stack with the stock Acidanthera build** unless you intentionally want to remove FocalTech support.
>
> This fork intentionally contains a patched `VoodooPS2Controller.kext` together with the separate `VoodooPS2FocalTech.kext`. Because the controller binary is modified, its **file size and hash/MD5 can differ from the stock Acidanthera VoodooPS2 binary even when the displayed version number is the same**. OCAT may therefore mark the stock package as an available update. **That difference is expected and is not an error.**
>
> Replacing this fork with the stock Acidanthera VoodooPS2 package can remove the FocalTech controller compatibility path and leave the trackpad non-functional. Update this FocalTech stack from **this repository's Releases** instead.

This warning applies specifically to the VoodooPS2/FocalTech entries. Other unrelated OCAT updates can still be handled normally.

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

## Releases vs. Actions artifacts

GitHub Actions **artifacts** are build outputs used for continuous integration and development verification. They are not the long-term public distribution channel.

Stable, user-facing binaries are published under **Releases** as both RELEASE and DEBUG packages.

### Release numbering policy

The public release number follows the current official **Acidanthera VoodooPS2 release line**:

- Acidanthera `2.3.7` → VoodooPS2-FocalTech `2.3.7`
- if this fork needs an independent hotfix before Acidanthera publishes another version, use `2.3.7-focaltech.1`, `2.3.7-focaltech.2`, etc.
- when Acidanthera publishes a new VoodooPS2 release, review/merge the upstream changes, rebuild and validate the FocalTech stack, then publish the corresponding release number here.

Release numbers intentionally track upstream, but a FocalTech release is **not claimed to be byte-for-byte identical** to the Acidanthera binary: it contains the FocalTech-specific additions and compatibility changes documented in this repository.

## Build

Precompiled packages are available under Releases. Building from source is intended for development.

Requirements: macOS, full Xcode, Git and Internet access.

Release build:

```bash
chmod +x build_focaltech.sh
./build_focaltech.sh
```

Debug build:

```bash
BUILD_CONFIGURATION=Debug ./build_focaltech.sh
```

The build is done in an isolated temporary copy and produces a complete FocalTech stack containing:

```text
VoodooPS2Controller.kext
├── Contents/PlugIns/VoodooPS2Keyboard.kext
└── Contents/PlugIns/VoodooInput.kext

VoodooPS2FocalTech.kext
```

The script applies the FLT0102 controller compatibility patch only in the temporary build tree. If upstream changes the expected controller startup blocks, the patch **fails closed** and asks for review instead of producing an unreviewed binary.

## OpenCore order

Under `Kernel -> Add`:

1. `VoodooPS2Controller.kext`
2. `VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext`
3. `VoodooPS2FocalTech.kext`
4. `VoodooPS2Controller.kext/Contents/PlugIns/VoodooInput.kext`

See [Docs/INSTALLATION.md](Docs/INSTALLATION.md) for hardware-specific settings and the OCAT warning.

## Upstream synchronization

This repository is a fork of Acidanthera/VoodooPS2. Upstream changes are followed and reviewed so the FocalTech additions remain maintainable. Before a public release, upstream changes affecting the controller, ApplePS2MouseDevice, trackpad lifecycle or VoodooInput path should be reviewed and the resulting build validated on available FocalTech hardware.

The FocalTech controller patch is deliberately small and guarded so upstream changes are easy to review and incompatible startup changes fail closed during the build.

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
