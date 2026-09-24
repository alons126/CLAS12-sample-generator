# Validation and legacy parity

## 1. Meaning of parity

Three different contracts are checked independently:

1. **LUND records:** matching headers, particle identities, momenta, and target vertices for matched seeds, beam/geometry/A/Z, and sampling settings. The pinned upstream ep/en profile maps to `--hadron-momentum uniform --hadron-p-min 0.3`; the upper bound is automatically beam energy. Mass/energy fields use the protected target source through the maintained adapter, and the tester now samples its target geometry.
2. **Uniform diagnostics:** archived 1e histogram names, entries, contents, errors, and most axes remain the reference. Vertex-z histograms deliberately use the wider −7.5–5 cm display needed by the maintained target catalog instead of the archived −5–5 cm axis. Electron-hadron monitoring adds the hadron species and FD/CD region to names and titles, and ends each object name with the complete resolved channel so ROOT statistics boxes identify the sample correctly. All uniform modes are checked for one nonduplicated monitoring file.
3. **Job arguments:** matching GEMC/reconstruction arguments and output naming at matched field/card/YAML/file-count settings. Real detector execution is a separate validation stage.

The maintained production electron/proton mixtures and zero-to-beam uniform neutron mode intentionally differ from the pinned upstream submodule's older momentum bounds and modes. The production modes have their own analytical distribution tests; fixed 1 GeV/c remains a neutron-only option.

Uniform FD pion modes and every uniform CD mode currently have structural integration coverage only. They have not yet been tested as production samples through full distribution and detector-workflow validation; see the explicit status note in [uniform samples](../create-lund/uniform.md).

## 2. Independent references

`legacy_uniform_driver.cpp` includes event functions, tester and histogram initialization from the pinned `legacy/Uniform-sample-generator` submodule. It supplies missing compilation context, output streams, target/channel parameters and deterministic RNG seeds. It bypasses launcher cleanup and large production defaults. The production executable and reference do not share sampling or LUND-writing implementations. Initialize the submodule before configuring tests. Exact comparisons explicitly select the matching upstream ep/en momentum and angular modes.

`prepare_legacy_genie.py` generates a test-only copy of the archived converter. Changes are limited to resolving includes, replacing external output setup with a new temporary directory, safe directory creation, and retaining its original diagnostic output inside the isolated reference. The maintained physical converter creates no monitoring histograms. This comparison is development-only; the production contract and its explanations stand independently of the reference.

Archived files remain untouched. Reference binaries are not installed and tests do not submit real jobs.

## 3. Reproduction

```bash
cmake -S . -B build/debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/debug --parallel 4
ctest --test-dir build/debug --output-on-failure
```

For a focused audit:

```bash
ctest --test-dir build/debug --output-on-failure -R 'legacy-parity|uniform-distributions'
```

The full suite currently registers nine tests when both workflows are enabled and csh/tcsh is available (eight without the shell). Temporary directories isolate generated fixtures and output. Successful test output is the current executable evidence; rerun after changing the reference sources or production algorithms.

## 4. Matrix and criteria

| Test | Cases | Pass criterion |
| --- | --- | --- |
| `uniform-integration` | 1e plus all proton/neutron/pip/pim FD/CD labels, seed repeat, tester, config overrides, invalid inputs | Output counts/PDGs, bounds, one monitoring ROOT file, rounded-record mass-shell relation, unchanged repeat output and rejection semantics |
| `genie-gst-integration` | Processes/species, follow-up-file submission cutoff, exact completed block, capacity limit, missing/wrong branches, mismatched arrays, empty/unsupported input, 300 supported particles, broken later chain file | Correct records/counts, no mid-file cutoff, complete traversal of valid arrays, malformed-array rejection, no monitoring ROOT files, and no completed manifest on failures |
| `uniform-legacy-parity` | 1e/ep/en/tester at 2.07052/4.02962/5.98636 GeV; additional target geometries | Exact LUND bytes with matching upstream settings; archived 1e histogram numerics apart from the maintained wider vertex-z axis; intentional FD/CD naming for hadrons |
| `genie-gst-legacy-parity` | 10000 accepted events at each reference energy and associated C12 geometry; supported species/processes plus a residual pi0; short fixture | Exact retained fields including mass-derived energy after removing the intentionally unsupported reference pi0, no maintained physical monitoring file, and explicit verification that the corrected short-input behavior does not reproduce the archived premature stop |
| `uniform-distributions` | 20000 default 1e events plus 20000 sampled enFD and epFD events | Bounds and empirical-CDF distance <0.025 for electron/proton mixture components, uniform neutron momentum, phi and flat theta |
| `submission-input-resolution` | Real uniform output plus portable/manual fixtures, defaults and overrides | Correct counts, GEMC 5.14 fallback, config/CLI precedence, conflict and malformed-input rejection |
| `submission-legacy-parity` | Legacy uniform/physical setup at 2/4/6 GeV, FC labels, FD/CD channels, failures | Exact full stdout and array/environment handoff, one array per sample, safe failure and shell survival |

Uniform reference seeds are kinematic 67890 and vertex 12345. Target checks cover Ar plus liquid, 4-foil, 1-foil, 1-foil-small, 1-foil-large and Ca; maintained tester events use the configured target geometry. Photon, charged pions, nucleons, one residual neutral pion and an unrelated kaon are included in the GENIE fixture. The maintained converter must retain the supported detector-stable particles and photon, while skipping both PDG 111 and the unrelated species. The comparison removes the reference pi0 record, adjusts multiplicity and particle indices, and then requires every retained field, including mass and energy, to match.

CDF tests independently evaluate the formulas in [sampling models](../concepts/sampling-models.md). The ep test checks the uniform-p subsequence, uniform-1/p subsequence and the combined mixture; the en test checks flat theta, phi and p. A fixed numerical threshold is used as a regression criterion, not as a formal significance claim across arbitrary seeds.

The `replacement-geometry` test verifies header-only geometry updates, new target names and RNG independence. The `ssh-launcher` test verifies sourced-shell survival/status, argument quoting, settings, build invocation and safe updates against a local Git fixture. Neither requires a remote server.

## 5. Intentional differences and limits

- **Physical-input cutoff:** before starting a follow-up file, conversion requires at least `events-per-file` inclusive input entries beginning with the current accepted entry. The first file is allowed through input exhaustion, and an exact final block is never interrupted after it starts. This remains an input-entry cutoff, not an accepted-event calculation.
- **Unknown historical RNG state:** archived `TRandom3(0)` runs cannot be reconstructed from a seed that was never recorded. Deterministic parity uses an explicitly substituted nonzero seed.
- **Mass source:** electron, proton, neutron, and charged-pion masses are read from protected [`targets.h`](../../src/lund-generation/external/targets.h); photons use exact zero. Tests require the serialized mass and derived energy fields.
- **Neutral pions:** the archived converter copied PDG 111 directly. The maintained converter requires neutral pions to be decayed during upstream GENIE production and consumes the resulting photons; residual PDG 111 entries are skipped because this adapter does not generate missing decay kinematics.
- **Production sampling modes:** the 1e/ep mixtures and en zero-to-beam uniform momentum follow the requested acceptance-map coverage and differ from upstream unless its settings are selected explicitly.
- **Diagnostics and paths:** monitoring is uniform-only and stored once in `<prefix>_monitoring_plots.root`. Legacy 1e definitions remain the reference except that every vertex-z histogram spans −7.5–5 cm so checked-in target geometries are visible; hadron definitions add the requested species/region notation. The generation log adds provenance.
- **Metadata:** ready-made Ar examples use A=40/Z=18, while the original uniform launcher used A=Z=1. Match settings explicitly; `legacy-coderun.conf` captures the archived values.
- **Detector processing:** local executable stubs verify command contracts only. GEMC/server software, geometry databases, reconstruction versions, field settings and simulation RNG state are required before claiming identical HIPO or acceptance results.

## 6. Server validation for a technical note

Use a small matched legacy/current LUND sample, identical GEMC/reconstruction versions, identical card/YAML/database resources and explicit detector RNG control if supported by the production setup. Compare event counts, generated banks, reconstructed particle yields and acceptance distributions, retaining logs and resource hashes. Document tolerances and statistical uncertainties. No server jobs are submitted by the repository's tests.

The submission parity test uses isolated checkouts and captures inert Slurm calls. Golden reports captured from the working C-shell coordinator cover both sources in preview and execute modes; only checkout paths, ANSI escapes and platform-specific `wc` padding are normalized. Fifteen source/energy/channel cases check array arguments and configured exports. Manifest-driven submission is tested for both sources. Additional cases cover channel extensions, shorter final files with a shared event limit, multiple samples, output replacement, non-destructive preview and failures. No actual Slurm or detector jobs run during these tests. Input EOF behavior with the selected detector software remains part of server validation.
