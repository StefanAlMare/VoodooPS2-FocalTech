# Release policy

## Development artifacts

The latest stable FocalTech release remains **2.3.7**. Builds from a branch or `master` are development artifacts even when compiled with the Release configuration. Release means optimized compilation; it does not mean hardware validation.

The current upstream controller/keyboard project reports the unreleased development version **2.3.8**. The standalone FocalTech bundle reports **2.3.7** and VoodooInput stays pinned to public **1.1.6**. These independent component versions are recorded in `BUILD-METADATA.json`; they must not be used to claim a new public upstream release.

## Preparing a candidate

1. Review the latest official upstream release and master changes, including controller startup, ApplePS2MouseDevice/ApplePS2Device, power transitions, VoodooInput and build dependencies. Record exact revisions and any compatibility patch changes.
2. Require successful stock CI and standalone FocalTech Release/Debug jobs for the candidate commit. Preserve logs, `BUILD-METADATA.json` and source SHA. A dirty local build is useful for development, but is not sufficient release evidence.
3. Run **Prepare draft release** manually for the selected source ref and a new version. Follow the latest public upstream release line, using `2.3.7-focaltech.N` for fork updates while upstream remains at 2.3.7. A development version bump in upstream master does not authorize publishing 2.3.8 here.
4. The workflow checks the version, refuses an existing release, runs patch tests, builds both configurations and creates a **draft** containing both packages and checksums. It never publishes automatically; editing `RELEASE_VERSION` has no publication effect. `RELEASE_VERSION` records the stable release and stays unchanged until promotion.

## Promoting a stable release

A maintainer must review the candidate's exact commit and artifacts before manually publishing the draft. Record hardware reports against the candidate source SHA and build metadata, covering:

- FLT0101 on its muxed topology, without `foclegacy` or `focfte`.
- FLT0102 on its simple AUX topology, with `foclegacy=1` and without `focfte`.
- Keyboard and pointer after cold boot, restart and shutdown; sleep/wake recovery; multitouch gestures, physical/secondary clicks, dragging and Force Click behavior on the macOS versions claimed in the release notes.

Old hardware reports do not validate new artifacts. If candidate validation is incomplete, keep it as a draft or explicitly publish it as a prerelease; do not call it stable. Update the draft notes with actual candidate evidence before promotion. This review does not publish any new release.

**FLT0103 remains experimental. FTE0001 remains experimental and requires `focfte=1`; a successful build or FLT test never validates FTE0001.** Its single-AUX restriction remains in place. Any later promotion of FTE0001 requires separate physical-device evidence and an explicit documentation change.

The generic upstream CI workflow provides stock-driver test artifacts only and must not attach them to FocalTech releases. Stable packages must come from `build_focaltech.sh` so the patched controller, standalone client, keyboard and pinned VoodooInput stay together.
