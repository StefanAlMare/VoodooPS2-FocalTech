"""Exercise the build-time patch on a disposable copy, never the checkout."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "Scripts/apply-controller-compat.py"
CONTROLLER = Path("VoodooPS2Controller/VoodooPS2Controller.cpp")


class ControllerCompatibilityTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.target = self.root / CONTROLLER
        self.target.parent.mkdir()
        self.original = (ROOT / CONTROLLER).read_text()
        self.target.write_text(self.original)

    def apply(self):
        return subprocess.run([sys.executable, str(SCRIPT)], cwd=self.root,
                              text=True, capture_output=True)

    def test_patch_applies_and_is_idempotent(self):
        result = self.apply()
        self.assertEqual(result.returncode, 0, result.stderr)
        patched = self.target.read_text()
        self.assertIn('PE_parse_boot_argn("foclegacy"', patched)
        self.assertIn('else if (_resetControllerFlag & RESET_CONTROLLER_ON_BOOT)', patched)
        self.assertNotIn('PE_parse_boot_argn("focfte"', patched)
        self.assertEqual(self.apply().returncode, 0)
        self.assertEqual(self.target.read_text(), patched)

    def test_changed_startup_blocks_fail_without_partial_write(self):
        for anchor in [
            '  PE_parse_boot_argn("ps2rst", &_resetControllerFlag, sizeof(_resetControllerFlag));',
            '    resetDevices();\n    flushDataPort();',
        ]:
            with self.subTest(anchor=anchor):
                self.assertEqual(self.original.count(anchor), 1)
                changed = self.original.replace(anchor, anchor + " // upstream changed")
                self.target.write_text(changed)
                self.assertNotEqual(self.apply().returncode, 0)
                self.assertEqual(self.target.read_text(), changed)

    def test_duplicate_startup_block_fails(self):
        self.target.write_text(self.original + self.original)
        self.assertNotEqual(self.apply().returncode, 0)
        self.assertEqual(self.target.read_text(), self.original + self.original)


if __name__ == "__main__":
    unittest.main()
