# Piano Auto Sustain 2.1 Zero-Setup Recording Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Windows recorder that publishes processed notes and CC64 through a virtual MIDI endpoint so Cubase and other hosts can monitor and record the final performance stream.

**Architecture:** Keep `PedalEngine` and `MidiBridgeEngine` as the platform-neutral processing path. Add a capability-selected output backend: Windows MIDI Services Virtual Device when a production-capable runtime is installed, otherwise an explicitly selected external virtual MIDI port. A recorder controller owns device selection, persistence, safety release, and UI state; Cubase presets/documentation provide the final one-time track-input binding.

**Tech Stack:** C++17, JUCE 8.0.12, CMake 3.22+, Windows MIDI Services WinRT API when available, WinMM/JUCE compatibility output, Google-free local CTest executables and PowerShell packaging tests.

**Spec:** `docs/superpowers/specs/2026-10-11-zero-setup-recording-design.md`

## Global Constraints

- Public product/file version is `2.1.0`; UI version is `2.1`; brand is `Amu music`.
- Windows x64 is the only binary platform claimed for 2.1.0.
- Do not redistribute unsigned preview drivers or third-party virtual MIDI drivers.
- Use Windows MIDI Services only when runtime detection and `MidiVirtualDeviceManager::IsTransportAvailable` both succeed; otherwise use the external-port compatibility backend.
- Keep the Cubase MIDI module class ID and persistent state UID unchanged.
- Recorder owns automatic CC64; do not enable the Cubase MIDI insert on the same Recorder-fed track.
- Every stop, disconnect, backend switch, shutdown, or message-storm condition must attempt CC64=0 and All Notes Off on all 16 channels.
- Configuration writes must use temporary-file plus atomic replacement and must not contain performance notes or project names.
- Preserve Chinese/English UI, five themes, and 100%/150%/200% DPI behavior.

## Review Focus

- A physical keyboard appears twice or shares a display name: reconnect only by stable identifier and require selection when identity is ambiguous (Task 4 tests).
- A host exposes the dynamic virtual endpoint through WinMM late or renumbers it: wait for the client endpoint and report readiness without guessing another port (Task 3 tests).
- The user selects the same physical/virtual endpoint as both sides: reject before opening and send nothing (Task 2 tests).
- Shutdown occurs after output failure: retain a pending safety-release state and retry on reconnection before forwarding notes (Task 2 tests).
- CC64 and Note On have the same timestamp: emit the required CC64 transition first and keep every other message stable (Task 2 tests).

---

### Task 1: Establish the 2.1 Version and Backend Contract

**Files:**
- Modify: `cmake/ProductVersion.cmake`
- Create: `Source/Recorder/VirtualMidiBackend.h`
- Create: `Source/Recorder/RecorderStatus.h`
- Create: `tests/RecorderContractTests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/VersionConfigTest.cmake`

**Interfaces:**
- Produces: `pas::VirtualMidiBackend` with `Availability probe()`, `bool start(std::string_view endpointName)`, `void stop()`, `bool send(MidiEvent)`, `bool clientConnected() const`, and `std::string endpointIdentifier() const`.
- Produces: `pas::RecorderStatus` and enums `BackendKind`, `ConnectionState`, `RecorderError` shared by controller and UI.

- [ ] **Step 1: Write failing contract and version tests** asserting 2.1.0/2.1, endpoint name `Piano Auto Sustain`, explicit unavailable/error states, and no default backend success.
- [ ] **Step 2: Run** `cmake -S . -B <core-build> -DBUILD_PLUGIN=OFF -DBUILD_NATIVE_MIDI_PROBE=OFF && cmake --build <core-build> --config Release && ctest --test-dir <core-build> -C Release -R "product_version|recorder_contract" --output-on-failure`; expect version/contract failure.
- [ ] **Step 3: Implement the minimal interface types and update version constants** without platform code.
- [ ] **Step 4: Re-run the focused tests**, expecting both to pass.
- [ ] **Step 5: Commit** `build: establish recorder 2.1 contracts`.

### Task 2: Make the Processing Bridge Produce a Recordable Stream

**Files:**
- Modify: `Source/Bridge/BridgeState.h`
- Modify: `Source/Bridge/MidiBridgeEngine.h`
- Modify: `Source/Bridge/MidiBridgeEngine.cpp`
- Modify: `tests/MidiBridgeTests.cpp`

**Interfaces:**
- Consumes: `pas::MidiEvent`, `pas::PedalEngine`, existing `IMidiInput`/`IMidiOutput`.
- Produces: `enum class HardwarePedalPolicy { Automatic, Physical, Merge }`, `BridgeMetrics`, `void panic()`, and ordered output through `IMidiOutput::send(MidiEvent)`.

- [ ] **Step 1: Add failing tests** for CC64-before-Note-On ordering, preservation of non-CC64 messages, three hardware-pedal policies, same-endpoint rejection, output loss/retry, all-channel panic, and message-storm shutdown.
- [ ] **Step 2: Run** `ctest --test-dir <core-build> -C Release -R midi_bridge --output-on-failure`; expect the new assertions to fail.
- [ ] **Step 3: Implement the smallest engine changes** using one ordered output path and a pending-release flag; do not add UI or device enumeration.
- [ ] **Step 4: Run all core tests**, expecting no regressions in pedal/chord/native timeline behavior.
- [ ] **Step 5: Commit** `feat: produce record-ready MIDI and pedal stream`.

### Task 3: Add the Windows Virtual MIDI Services Backend

**Files:**
- Create: `Source/Recorder/WindowsMidiServicesBackend.h`
- Create: `Source/Recorder/WindowsMidiServicesBackend.cpp`
- Create: `Source/Recorder/WindowsMidiServicesApi.h`
- Create: `tests/WindowsMidiBackendTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `VirtualMidiBackend` from Task 1.
- Produces: `WindowsMidiServicesBackend` and injectable `WindowsMidiServicesApi` covering runtime initialization, transport availability, virtual-device creation, endpoint send, client-in-use status, and shutdown.

- [ ] **Step 1: Add failing tests with a fake API boundary** for missing runtime, disabled transport, creation failure, delayed client endpoint, client connect/disconnect, MIDI 1.0-to-UMP conversion, and deterministic close order.
- [ ] **Step 2: Run** the new backend test target; expect failure because the backend does not exist.
- [ ] **Step 3: Implement runtime-loaded WinRT integration** so unsupported systems load the executable without missing-DLL failure. Use the one-message WinRT path required for virtual-device discovery and the service-provided client endpoint.
- [ ] **Step 4: Run backend tests and a local capability probe**; record the installed runtime/transport result without treating unavailable preview components as test failure.
- [ ] **Step 5: Commit** `feat: add Windows MIDI Services virtual endpoint`.

### Task 4: Add Device Selection, Auto-Reconnect, and Persistent Configuration

**Files:**
- Create: `Source/Recorder/RecorderConfig.h`
- Create: `Source/Recorder/RecorderConfig.cpp`
- Create: `Source/Recorder/DeviceSelection.h`
- Create: `Source/Recorder/DeviceSelection.cpp`
- Create: `Source/Recorder/RecorderController.h`
- Create: `Source/Recorder/RecorderController.cpp`
- Create: `tests/RecorderControllerTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `VirtualMidiBackend`, `MidiBridgeEngine`, `RecorderStatus`.
- Produces: `RecorderController::refreshDevices(std::vector<DeviceInfo>)`, `selectInput(std::string id)`, `start()`, `stop()`, `panic()`, `status()`, `metrics()`; `RecorderConfigStore::load/save` with schema version 3.

- [ ] **Step 1: Add failing tests** for unique-device auto-selection, multiple-device ambiguity, stable-ID reconnect, renamed-device display, missing-device wait, backend fallback, retry-before-forward, corrupt JSON backup, atomic save, and migration from 2.0 Bridge settings.
- [ ] **Step 2: Run** `ctest --test-dir <core-build> -C Release -R recorder_controller --output-on-failure`; expect failure.
- [ ] **Step 3: Implement controller and config store** with dependencies injected for filesystem, input enumeration, and backend creation.
- [ ] **Step 4: Run the controller test plus complete core suite**, expecting all pass.
- [ ] **Step 5: Commit** `feat: add recorder auto-connect and settings`.

### Task 5: Build the Recorder User Interface and First-Run Flow

**Files:**
- Create: `Source/Recorder/RecorderApplication.cpp`
- Create: `Source/Recorder/RecorderPanel.h`
- Create: `Source/Recorder/RecorderPanel.cpp`
- Modify: `Source/UI/MainPanel.h`
- Modify: `Source/UI/MainPanel.cpp`
- Modify: `Source/UI/Theme.h`
- Create: `tests/RecorderUiSmoke.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `RecorderController`, `RecorderStatus`, `BridgeMetrics`.
- Produces: GUI target `PianoAutoSustainRecorder`; first-run states `ChooseKeyboard`, `InstallMidiRuntime`, `ChooseCompatibilityOutput`, `SelectHostInput`, `Ready`.

- [ ] **Step 1: Add failing headless UI tests** for Chinese/English strings, five themes, connection-state mapping, single-device zero-click path, multi-device choice, runtime-install guidance, panic action, explicit start-with-Windows opt-in, remembered close-to-tray choice, and 100%/150%/200% snapshots without clipped controls.
- [ ] **Step 2: Run** the UI smoke target; expect failure because Recorder UI is absent.
- [ ] **Step 3: Implement the minimal UI** with performance controls on the main screen and routing/launch/pedal policy in Advanced Settings. Never claim “ready” until input and output are open.
- [ ] **Step 4: Run UI tests and inspect all generated snapshots**, correcting layout until all three scales are readable.
- [ ] **Step 5: Commit** `feat: add guided recorder application`.

### Task 6: Provide the External-Port Compatibility Backend

**Files:**
- Create: `Source/Recorder/JuceVirtualPortBackend.h`
- Create: `Source/Recorder/JuceVirtualPortBackend.cpp`
- Modify: `Source/Recorder/RecorderController.cpp`
- Create: `tests/CompatibilityBackendTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `VirtualMidiBackend`, JUCE MIDI output enumeration.
- Produces: backend that uses an existing external virtual port selected by stable identifier; it never installs or creates a driver.

- [ ] **Step 1: Add failing tests** for no ports, one saved port, ambiguous names, physical-output exclusion, same-input rejection, disconnect/reconnect, and safe fallback from unavailable Windows MIDI Services.
- [ ] **Step 2: Run** the compatibility backend tests; expect failure.
- [ ] **Step 3: Implement JUCE output backend and controller selection policy**; show an official-runtime link or explicit external-port choice rather than silently choosing.
- [ ] **Step 4: Run all recorder and bridge tests**, expecting pass.
- [ ] **Step 5: Commit** `feat: add compatible virtual-port fallback`.

### Task 7: Add Cubase Preset/Template and End-to-End Recording Validation

**Files:**
- Create: `assets/cubase/README.md`
- Create after Cubase export: `assets/cubase/Piano Auto Sustain Recorder.trackpreset` or `assets/cubase/Piano Auto Sustain Recorder.cpr`
- Create: `tests/RecordingAcceptanceChecklist.md`
- Create: `tests/RecordedMidiInspection.cpp`
- Modify: `docs/COMPATIBILITY.md`

**Interfaces:**
- Consumes: Recorder endpoint named `Piano Auto Sustain`.
- Produces: user-loadable Cubase asset and a MIDI-file inspector that asserts note events plus CC64 127/0 exist in an exported test recording.

- [ ] **Step 1: Write the acceptance checklist and failing MIDI inspection test** using a fixture with notes but no CC64.
- [ ] **Step 2: Run the inspector**, expecting `CC64 down/up missing`.
- [ ] **Step 3: Export a minimal Cubase preset/template** with the virtual input selected and no Cubase MIDI insert; document the fallback when Cubase refuses stable endpoint binding.
- [ ] **Step 4: Record the fixed performance fixture through Cubase**, export Standard MIDI, and run the inspector; expect notes, CC64=127, CC64=0, and no duplicate Note On at identical channel/pitch/time.
- [ ] **Step 5: Complete manual checks** for live Kontakt sound, Recorder-closed playback, device replug, Stop/Panic, and project reopen.
- [ ] **Step 6: Commit** `test: validate Cubase note and CC64 recording`.

### Task 8: Update Installer, Package, and Upgrade Cleanup

**Files:**
- Modify: `scripts/package-release.ps1`
- Modify: `tests/PackageLayoutTests.ps1`
- Modify: `README.md`
- Modify: `docs/COMPATIBILITY.md`
- Create: `docs/QUICKSTART-zh.md`
- Create: `docs/QUICKSTART-en.md`
- Modify: `Source/NativeMidi/Version.rc`

**Interfaces:**
- Consumes: all Release artifacts and Cubase asset from Tasks 3–7.
- Produces: `Piano Auto Sustain 2.1 Windows x64` folder/ZIP and clean source ZIP.

- [ ] **Step 1: Extend failing package tests** to require Recorder EXE, 2.1 versions, Cubase asset, both quick starts, backend guidance, licenses, SHA256, and removal instructions for 2.0/older filenames.
- [ ] **Step 2: Run package tests against the 2.0 package**, expecting missing 2.1 artifacts/version failures.
- [ ] **Step 3: Update resources, installer and docs**. Installer removes only known obsolete product files, preserves user projects/config backups, and never installs an unsigned MIDI driver.
- [ ] **Step 4: Build native DLL, VST3 and Recorder from fresh directories; run every CTest suite**.
- [ ] **Step 5: Generate the package, independently extract it, verify every SHA256 entry and Windows file/product version**, then scan for stale public 2.0/0.x branding.
- [ ] **Step 6: Move the 2.0 release/source archives to `04_历史备份` and commit** `release: package Piano Auto Sustain 2.1`.

### Task 9: Final Review and Release Evidence

**Files:**
- Create: `docs/RELEASE-2.1-VALIDATION.md`
- Update only if review finds defects: files owned by Tasks 1–8

**Interfaces:**
- Consumes: committed implementation and fresh release package.
- Produces: traceable release evidence with automated results, host matrix, hashes, known limitations, and exact tested versions.

- [ ] **Step 1: Run a requirements audit** mapping every section of the design spec to code, tests, documentation or an explicit unsupported-platform statement.
- [ ] **Step 2: Run a fresh reviewer pass** focused on real-time safety, endpoint lifetime, duplicated MIDI, configuration corruption, shutdown order, installer cleanup and truthfulness of compatibility claims.
- [ ] **Step 3: Fix blocking findings with a failing regression test first**, then rerun the affected suite and full suite.
- [ ] **Step 4: Repeat the Cubase live-recording acceptance** and record the exported MIDI inspector result.
- [ ] **Step 5: Write validation evidence, verify clean Git state, and commit** `docs: record Piano Auto Sustain 2.1 validation`.
