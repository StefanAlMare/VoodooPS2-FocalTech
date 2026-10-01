# Upstream review — 2026-10-01

## Public releases and ancestry

- [Acidanthera VoodooPS2 2.3.7](https://github.com/acidanthera/VoodooPS2/releases/tag/2.3.7), published 2024-12-03, is still the latest public release. Its source commit is `bdbf80639936701337f4574bae8c3ea25bb8dd3d`.
- [Upstream master](https://github.com/acidanthera/VoodooPS2/commit/7eab4a344d901d1e4aa6ece4189ecde5a7356287) is `7eab4a344d901d1e4aa6ece4189ecde5a7356287`.
- The inspected fork master was `403c2a4880805419b5027d3cc6ed003c795ae735`. `git merge-base` with upstream returned the upstream tip; `git log HEAD..upstream/master` was empty. All current upstream changes are already integrated. No merge or cherry-pick is needed.
- The fork's published [2.3.7](https://github.com/StefanAlMare/VoodooPS2-FocalTech/releases/tag/2.3.7) remains unchanged. Fork and upstream tags with the same name refer to different commits; compare the upstream commit explicitly instead of replacing the fork tag.

## Changes since upstream 2.3.7

| Commit | Change | Impact on FocalTech |
| --- | --- | --- |
| [`3175575`](https://github.com/acidanthera/VoodooPS2/commit/3175575e19cf0e8ce8b3789f33ffc4b280f086ce) | Project version 2.3.7 → 2.3.8 | Development bundle version only; no published 2.3.8 release |
| [`b6713c3`](https://github.com/acidanthera/VoodooPS2/commit/b6713c37b9919bd95c3b7f4d484bd049371bcfc1) | CI badge URL | Documentation only |
| [`7eab4a3`](https://github.com/acidanthera/VoodooPS2/commit/7eab4a344d901d1e4aa6ece4189ecde5a7356287) | Checkout v5, artifact upload v4, release upload action update | CI infrastructure; already present in fork |

`git diff bdbf806..upstream/master` changes only the project version, upstream README and CI workflow. In particular:

- **VoodooPS2Controller:** no post-release startup, mux, interrupt, power or transport changes. The `foclegacy` patch still applies to both exact startup blocks and leaves the upstream reset path selected when the option is absent.
- **ApplePS2MouseDevice / ApplePS2Device:** no source or interface changes. Both FocalTech backends keep the same provider and command transport.
- **Keyboard and stock trackpad drivers:** no post-release runtime changes. Existing 2.3.7 fixes for multiple PS2/SMBus attachment and PS/2 stub shutdown are already included.
- **Standalone build:** still reuses the stock trackpad target, substitutes the FocalTech translation unit and excludes the stock trackpad backends. Both FocalTech classes must be present in the final binary. Generic upstream CI does not exercise this substitution.
- **`focfte`:** remains an opt-in in `ApplePS2FTE0001::probe`, separate from the controller's `foclegacy` patch. Its single-AUX restriction and experimental status remain unchanged. No upstream change requires modifying either boot argument.

## VoodooInput

The latest public release is [1.1.6](https://github.com/acidanthera/VoodooInput/releases/tag/1.1.6), published 2024-10-08. [Comparison with master](https://github.com/acidanthera/VoodooInput/compare/1.1.6...d897813718c2ef9ad143b6a86240fe52dcb1693e) contains only `d897813`, which changes the project version to 1.1.7. No event structures, headers, message protocol or runtime code changed. Standalone builds continue using the 1.1.6 release binaries and their matching SDK headers for both configurations.

## Maintenance changes from this review

FocalTech CI now covers every PR and master push so controller/provider/build-system changes cannot bypass standalone checks. Patch regression tests verify repeat application and refusal of changed or ambiguous startup blocks. Package checks validate identifiers, actual versions, x86_64 binaries, both FocalTech personalities and the standalone plugin layout. `BUILD-METADATA.json` records the SDK revision and source state; MacKernelSDK still follows its upstream default branch, so builds are not claimed to be byte-reproducible.

The manual release workflow prepares a draft only after RELEASE and DEBUG succeed. Generic upstream CI cannot upload stock binaries into a FocalTech release. See [release policy](RELEASE-POLICY.md) for promotion requirements.

Compilation and automated checks do not exercise an i8042 controller or touchpad. The historical FLT0101/FLT0102 validation belongs to stable 2.3.7. New artifacts need regression testing; FLT0103 and FTE0001 remain experimental, and FTE0001 is not hardware-validated.

## Local verification

Both standalone **Release and Debug builds passed** on 2026-10-01 with Xcode 26.6 (17F113), MacKernelSDK `3f750085caa17ec3a7880f11c11bf4f48cd6a164`, and VoodooInput 1.1.6. Package checks passed for all four x86_64 bundles and both FocalTech personalities; binaries contain the `foclegacy` and `focfte` markers. Three controller compatibility tests, shell syntax and workflow YAML parsing also passed.

These local builds used the working changes on base `403c2a4` and correctly record `source_dirty: true`. GitHub CI supplies clean-commit validation and archived logs for the submitted revision. Xcode emitted warnings about legacy project settings/build phases, duplicate `-lkmod`, upstream `offsetof` usage and an unused ALPS variable; there were no compilation errors. No deployment target or runtime protocol change was made to suppress those warnings.
