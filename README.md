# VoodooPS2-FocalTech

A maintained fork of [Acidanthera/VoodooPS2](https://github.com/acidanthera/VoodooPS2) adding native **FocalTech PS/2 multitouch trackpad support** across two protocol families: **FLT0101 and FLT0102 are hardware-validated**, **FLT0103 is experimental**, and **FTE0001 is a separate experimental opt-in backend**.

The FocalTech driver reports through **VoodooInput / native macOS multitouch**, instead of implementing macOS gestures itself.

## Downloads

For validated FLT0101/FLT0102 systems, download a precompiled package from **[Releases](https://github.com/StefanAlMare/VoodooPS2-FocalTech/releases)**. You do **not** need Xcode or source compilation to install the Release package.

- **`VoodooPS2-FocalTech-<version>-RELEASE.zip`** — ready-to-use package, recommended for normal use on validated hardware.
- **`VoodooPS2-FocalTech-<version>-DEBUG.zip`** — development/diagnostics package for contributors, hardware bring-up and bug reports.
- **GitHub Actions artifacts** — current development builds. Use these when testing experimental support such as FTE0001 before it is promoted into a stable release.

Both stable packages include installation instructions, credits, source/build metadata and **`SUPPORTED-HARDWARE.md`**, which states the supported hardware and limitations. See [Docs/SUPPORTED-HARDWARE.md](Docs/SUPPORTED-HARDWARE.md) before trying the driver on unvalidated hardware.

## Status

| Hardware ID | Protocol family | Status | Controller layout / notes |
|---|---|---|---|
| **FLT0101** | FLT six-byte native | ✅ Hardware validated | i8042 active multiplexing / 4 AUX nubs; mux-safe path; no compatibility boot argument |
| **FLT0102** | FLT six-byte native | ✅ Hardware validated | simple i8042 AUX; use `foclegacy=1` |
| **FLT0103** | FLT six-byte native | 🧪 Experimental | protocol support expected, not hardware-validated yet |
| **FTE0001** | FTE 8/16-byte legacy | 🧪 Experimental / opt-in | separate backend; requires `focfte=1`; initially limited to a simple single-AUX topology |

Other/unknown FocalTech PS/2 devices may be tried for testing, but compatibility is **experimental until physically validated**. This fork is not intended for unrelated Synaptics, non-FocalTech ELAN/Elantech, ALPS, Sentelic or generic PS/2 touchpad/mouse hardware.

### macOS validation

The current stable FLT stack has been hardware-tested successfully on **macOS Sequoia** and **macOS Tahoe** on validated FLT0101 and FLT0102 systems.

The FTE0001 backend is currently **experimental and not yet hardware-validated in this fork**. See [Docs/FTE0001.md](Docs/FTE0001.md).

### Confirmed FLT functionality

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

### Experimental FTE0001 backend

FTE0001 is a separate FocalTech PS/2 protocol family. The new `ApplePS2FTE0001` backend implements the historically observed `58 00 05` product response, 8/16-byte packet framing, up to four contacts, hardware left/right buttons, native VoodooInput reporting and wake reinitialization.

It is deliberately disabled unless this boot argument is present:

```text
focfte=1
```

Do **not** use `focfte=1` on FLT0101 or FLT0102. While FTE0001 remains unvalidated, use a current DEBUG artifact and follow [Docs/FTE0001.md](Docs/FTE0001.md).

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
                 ├── ApplePS2FocalTech
                 │      └── FLT0101 / FLT0102 / FLT0103
                 │
                 └── ApplePS2FTE0001   [opt-in: focfte=1]
                        └── FTE0001

Both protocol backends
        │
        └── VoodooInput
                 │
                 └── macOS native multitouch
```

`VoodooPS2FocalTech.kext` is a separate client driver bundle containing independent FLT and FTE protocol backends. It intentionally depends on VoodooPS2Controller for PS/2 transport and on VoodooInput for multitouch reporting.

## Releases vs. Actions artifacts

GitHub Actions **artifacts** are build outputs used for continuous integration, development verification and experimental hardware testing. They are not the long-term public distribution channel.

Stable, user-facing binaries are published under **Releases** as both RELEASE and DEBUG packages.

Experimental code can live in `master` and CI artifacts without changing the compatibility claims of the latest stable release. FTE0001 will remain experimental until physical hardware validation is reported.

### Release numbering policy

The public release number follows the current official **Acidanthera VoodooPS2 release line**:

- Acidanthera `2.3.7` → VoodooPS2-FocalTech `2.3.7`
- if this fork needs an independent hotfix before Acidanthera publishes another version, use `2.3.7-focaltech.1`, `2.3.7-focaltech.2`, etc.
- when Acidanthera publishes a new VoodooPS2 release, review/merge the upstream changes, rebuild and validate the FocalTech stack, then publish the corresponding release number here.

Release numbers intentionally track upstream, but a FocalTech release is **not claimed to be byte-for-byte identical** to the Acidanthera binary: it contains the FocalTech-specific additions and compatibility changes documented in this repository.

## Build

Precompiled stable packages are available under Releases. Building from source is intended for development.

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

See [Docs/INSTALLATION.md](Docs/INSTALLATION.md) for hardware-specific settings, FTE0001 testing and the OCAT warning.

## Upstream synchronization

This repository is a fork of Acidanthera/VoodooPS2. Upstream changes are followed and reviewed so the FocalTech additions remain maintainable. Before a public release, upstream changes affecting the controller, ApplePS2MouseDevice, trackpad lifecycle or VoodooInput path should be reviewed and the resulting build validated on available FocalTech hardware.

The FocalTech controller patch is deliberately small and guarded so upstream changes are easy to review and incompatible startup changes fail closed during the build.

## Credits and provenance

This project exists because of work done by many people over many years. In particular:

- **Acidanthera and all VoodooPS2 contributors** for the modern VoodooPS2 controller and trackpad framework.
- **Apple** and the historical Apple PS/2 source on which the family of PS/2 drivers ultimately builds.
- **VoodooInput / VoodooI2C contributors** for Magic Trackpad 2 emulation and the native multitouch reporting path.
- **EMlyDinEsH**, author of **ApplePS2SmartTouchPad** and its historical ELAN/FocalTech/Synaptics work. ApplePS2SmartTouchPad was an essential practical reference while recovering FocalTech behaviour on hardware that modern VoodooPS2 did not handle.
- **chilledHamza**, author of the historical [`VoodooPS2FocalTech`](https://github.com/chilledHamza/VoodooPS2FocalTech) FTE0001 project. That project is an important public reference for the separate FTE0001 protocol family. The FTE0001 code in this APSL fork is an independent implementation rather than copied GPL source.
- **Linux kernel input developers**, particularly the contributors to the FocalTech PS/2 driver, for publicly documented FLT packet formats, device IDs and native protocol behaviour.
- The earlier **VoodooPS2 / ApplePS2 / RehabMan-era contributors** credited by upstream.
- **StefanAlMare** for the modern standalone FocalTech integration, controller compatibility work, mux-safe probing, native VoodooInput reporting, click-zone handling, progressive Force Click emulation and the independent experimental FTE0001 backend in this fork.

See [CREDITS.md](CREDITS.md) for a fuller attribution statement.

## License

The upstream repository is distributed under the **Apple Public Source License 2.0**. The original notices and `LICENSE.md` are retained. New FocalTech source in this fork is also distributed under APSL 2.0 unless a file explicitly states otherwise.

This is an independent community fork and is **not an official Acidanthera project**.
