import importlib.util
import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('pack', ROOT / 'scripts/package_portable.py')
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)

class Packaging(unittest.TestCase):
    def test_reject_wrong_platform_binary(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'plugin'
            binary.write_bytes(b'MZ' + bytes(126))
            with self.assertRaises(ValueError):
                pack.check_binary(binary, 'Linux', 'x64')
    def test_pe_architecture(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = pathlib.Path(tmp) / 'plugin'
            data = bytearray(256); data[:2] = b'MZ'; data[60:64] = (128).to_bytes(4,'little')
            data[128:132] = b'PE\0\0'; data[132:134] = (0x8664).to_bytes(2,'little')
            binary.write_bytes(data)
            pack.check_binary(binary, 'Windows', 'x64')
            with self.assertRaises(ValueError): pack.check_binary(binary, 'Windows', 'arm64')
    def test_manifest_detects_change(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp); (root/'plugin').write_bytes(b'original')
            pack.write_manifest(root); pack.verify_manifest(root)
            (root/'plugin').write_bytes(b'corrupt')
            with self.assertRaises(ValueError): pack.verify_manifest(root)
    def test_manifest_rejects_escape(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp); (root/'SHA256SUMS.txt').write_text('0'*64+'  ../outside\n')
            with self.assertRaises(ValueError): pack.verify_manifest(root)

if __name__ == '__main__': unittest.main()
