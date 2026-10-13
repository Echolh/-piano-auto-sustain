# Cross-platform packaging plan

Use the existing 1.0 processing core and keep the accepted Cubase DLL unchanged.

1. Add tested packaging contracts: platform/architecture checks, checksums, complete bundles and no overwriting output.
2. Add Windows VST3 transactional install/uninstall with rollback and file verification; test in an isolated destination.
3. Prepare separate AU MIDI FX buses/target for macOS; exercise MIDI-only processing on Windows before calling it a port candidate.
4. Add macOS/Linux user-level install/uninstall and CI build recipes. Build each system natively; never rename Windows binaries as Mac/Linux packages.
5. Rebuild/test Windows VST3, package with host-specific instructions, refresh source archives and manifest.
6. Report local validation separately from unrun Mac/Linux/DAW acceptance. User authorized private repository yuquny006-hash/amu-music-bridge branch piano-auto-sustain-1.0-build for source upload and GitHub Actions. Keep amu-piano unchanged.
