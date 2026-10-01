#!/usr/bin/env python3
"""Validate the assembled standalone stack and record its actual provenance."""
import json
from pathlib import Path
import plistlib
import re
import subprocess
import sys


def require(condition, message):
    if not condition:
        raise SystemExit(f"ERROR: {message}")


out, configuration, focal_version, input_version, source, sdk, dirty = sys.argv[1:]
out = Path(out)
controller = out / "Kexts/VoodooPS2Controller.kext"
plugins = controller / "Contents/PlugIns"
bundles = {
    "controller": controller,
    "keyboard": plugins / "VoodooPS2Keyboard.kext",
    "focaltech": out / "Kexts/VoodooPS2FocalTech.kext",
    "voodooinput": plugins / "VoodooInput.kext",
}
require({p.name for p in plugins.glob("*.kext")} ==
        {"VoodooPS2Keyboard.kext", "VoodooInput.kext"},
        "unexpected controller plugins in standalone package")
project = Path("VoodooPS2Controller.xcodeproj/project.pbxproj").read_text()
versions = set(re.findall(r"MODULE_VERSION = ([0-9.]+);", project))
require(len(versions) == 1, "controller project versions disagree")
controller_version = versions.pop()
expected = {
    "controller": ("as.acidanthera.voodoo.driver.PS2Controller", controller_version),
    "keyboard": ("as.acidanthera.voodoo.driver.PS2Keyboard", controller_version),
    "focaltech": ("com.stefanalmare.driver.VoodooPS2FocalTech", focal_version),
    "voodooinput": ("me.kishorprins.VoodooInput", input_version),
}
metadata = {}
for name, bundle in bundles.items():
    with (bundle / "Contents/Info.plist").open("rb") as file:
        info = plistlib.load(file)
    identifier, version = expected[name]
    require(info["CFBundleIdentifier"] == identifier, f"{name}: unexpected identifier")
    require(info["CFBundleVersion"] == version, f"{name}: unexpected bundle version")
    executable = bundle / "Contents/MacOS" / info["CFBundleExecutable"]
    archs = subprocess.check_output(["lipo", "-archs", str(executable)], text=True).split()
    require("x86_64" in archs, f"{name}: missing x86_64 executable")
    metadata[name] = {"identifier": identifier, "version": version, "architectures": archs}
    if name == "focaltech":
        personalities = info["IOKitPersonalities"].values()
        require({p["IOClass"] for p in personalities} ==
                {"ApplePS2FocalTech", "ApplePS2FTE0001"}, "missing or unexpected backend")
        require(all(p["IOProviderClass"] == "ApplePS2MouseDevice" for p in personalities),
                "unexpected FocalTech provider")
        require(not (bundle / "Contents/PlugIns/VoodooInput.kext").exists(),
                "duplicate VoodooInput in FocalTech bundle")

metadata = {
    "channel": "development",
    "source_commit": source,
    "source_dirty": bool(dirty),
    "configuration": configuration,
    "mackernelsdk_commit": sdk,
    "xcode": subprocess.check_output(["xcodebuild", "-version"], text=True).strip(),
    "bundles": metadata,
    "hardware_validation": "Not established by this build; FTE0001 remains experimental",
}
(out / "BUILD-METADATA.json").write_text(json.dumps(metadata, indent=2) + "\n")
print("Verified standalone package layout, versions, architecture and personalities.")
