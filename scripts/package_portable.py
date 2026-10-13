"""Native-platform bundles, manifests and installers. Never fabricate cross-platform binaries."""
import argparse
import hashlib
import json
import pathlib
import plistlib
import shutil
import struct
import subprocess
import tarfile
import tempfile
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]

def check_binary(path, system, arch):
    data = pathlib.Path(path).read_bytes()
    if system == 'Windows':
        if data[:2] != b'MZ' or len(data) < 64: raise ValueError('Not PE')
        offset = int.from_bytes(data[60:64], 'little')
        if data[offset:offset+4] != b'PE\0\0': raise ValueError('Missing PE header')
        machine = int.from_bytes(data[offset+4:offset+6], 'little')
        if machine != {'x64':0x8664, 'arm64':0xaa64}.get(arch): raise ValueError('PE architecture mismatch')
    elif system == 'Linux':
        if data[:6] != b'\x7fELF\x02\x01': raise ValueError('Not ELF64 little endian')
        if int.from_bytes(data[18:20], 'little') != 62 or arch != 'x64': raise ValueError('ELF architecture mismatch')
    elif system == 'macOS':
        if data[:4] not in (b'\xca\xfe\xba\xbe', b'\xca\xfe\xba\xbf'): raise ValueError('Expected universal Mach-O')
        count = struct.unpack_from('>I', data, 4)[0]
        size = 32 if data[:4] == b'\xca\xfe\xba\xbf' else 20
        cpus = {struct.unpack_from('>I', data, 8+i*size)[0] for i in range(count)}
        if not {0x01000007,0x0100000c}.issubset(cpus): raise ValueError('Missing Intel or Apple Silicon slice')
    else: raise ValueError('Unknown platform')

def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def write_manifest(root):
    root = pathlib.Path(root)
    rows = [f'{digest(p)}  {p.relative_to(root).as_posix()}' for p in sorted(root.rglob('*')) if p.is_file() and p.name != 'SHA256SUMS.txt']
    (root/'SHA256SUMS.txt').write_text('\n'.join(rows)+'\n', encoding='utf-8')

def verify_manifest(root):
    root = pathlib.Path(root).resolve()
    for row in (root/'SHA256SUMS.txt').read_text(encoding='utf-8-sig').splitlines():
        expected, name = row.split('  ',1)
        path = (root/name).resolve()
        if root not in path.parents: raise ValueError('Manifest escapes package')
        if digest(path) != expected: raise ValueError('Checksum mismatch: '+name)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--build',required=True);parser.add_argument('--output',required=True)
    parser.add_argument('--platform',choices=['Windows-x64','macOS-Universal','Linux-x64'],required=True)
    parser.add_argument('--commit',default='local');args=parser.parse_args()
    build=pathlib.Path(args.build).resolve();out=pathlib.Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
    system,arch=args.platform.split('-');name=f'Piano-Auto-Sustain-1.0-{args.platform}-TEST'
    suffix='.tar.gz' if system=='Linux' else '.zip'
    archive=out/(name+suffix)
    if archive.exists(): raise ValueError('Output already exists; refusing to overwrite')
    with tempfile.TemporaryDirectory(dir=out) as temporary:
        root=pathlib.Path(temporary)/name;root.mkdir()
        bundles=[build/'BassPedal_artefacts/Release/VST3/Piano Auto Sustain.vst3']
        if system=='macOS': bundles.append(build/'BassPedalAU_artefacts/Release/AU/Piano Auto Sustain MIDI FX.component')
        for bundle in bundles:
            if system=='macOS':
                info=plistlib.loads((bundle/'Contents/Info.plist').read_bytes());binary=bundle/'Contents/MacOS'/info['CFBundleExecutable']
            elif system=='Windows': binary=bundle/'Contents/x86_64-win/Piano Auto Sustain.vst3'
            else: binary=bundle/'Contents/x86_64-linux/Piano Auto Sustain.so'
            check_binary(binary,system,arch)
            shutil.copytree(bundle,root/bundle.name,symlinks=True)
        shutil.copytree(ROOT/'licenses',root/'licenses')
        for doc in ('VST3-GUIDE.md','HOST-GUIDE.md','COMPATIBILITY.md'):
            shutil.copy2(ROOT/'docs'/doc,root/doc)
        if system=='Windows':
            shutil.copy2(ROOT/'scripts/Install-VST3.ps1',root/'Install-VST3.ps1')
            for action in ('Install','Uninstall'):
                (root/(action+'.cmd')).write_text('@echo off\npowershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-VST3.ps1" -Action '+action+'\npause\n',encoding='ascii')
        else:
            for action in ('Install','Uninstall'):
                target=root/(action+'.command');shutil.copy2(ROOT/'scripts/portable-install.sh',target);target.chmod(0o755)
        (root/'BUILD-INFO.json').write_text(json.dumps({'product':'Piano Auto Sustain','version':'1.0.0','platform':args.platform,'commit':args.commit,'daw_acceptance':'pending','mac_signing':'ad-hoc; not notarized' if system=='macOS' else 'not applicable'},indent=2)+'\n')
        write_manifest(root);verify_manifest(root)
        if system=='macOS': subprocess.run(['ditto','-c','-k','--sequesterRsrc','--keepParent',str(root),str(archive)],check=True)
        elif system=='Linux':
            with tarfile.open(archive,'w:gz') as tf: tf.add(root,arcname=name)
        else:
            with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as zf:
                for p in root.rglob('*'):
                    if p.is_file():zf.write(p,p.relative_to(root.parent).as_posix())
    archive.with_name(archive.name+'.sha256').write_text(digest(archive)+'  '+archive.name+'\n')
    print(archive)
if __name__=='__main__': main()
