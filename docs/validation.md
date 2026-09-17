# Validation and legacy parity

## 1. Meaning of parity

Three different contracts are checked independently:

1. **LUND bytes:** identical serialized events for matched seeds, beam/geometry/A/Z, fixed legacy sampling and legacy precision/masses.
2. **Numerical diagnostics:** matching names, binning, entries, bin contents and errors, including flow bins. ROOT container bytes and rendered image styles may differ.
3. **Job arguments:** matching GEMC/reconstruction arguments and output naming at matched field/card/YAML/file-count settings. Real detector execution is a separate validation stage.

The requested non-fixed neutron and proton models intentionally differ from the archived fixed-1-GeV/c distribution. Their validation uses analytical distribution tests rather than equality to that fixed source.

## 2. Independent references

`legacy_uniform_driver.cpp` includes the archived event functions, tester and histogram initialization. It supplies missing compilation context, output streams, target/channel parameters and deterministic RNG seeds. It bypasses launcher cleanup and large production defaults. The production executable and reference do not share sampling or LUND-writing implementations.

`prepare_legacy_genie.py` generates a test-only copy of the archived converter. Changes are limited to resolving includes, replacing external output setup with a new temporary directory, safe directory creation, and capturing the original diagnostic histogram. The event loop, species/process selection, formatting and early-stop behavior remain unchanged. Thus tests reveal the short-input difference instead of modifying it out of the reference.

Archived files remain untouched. Reference binaries are not installed and tests do not submit real jobs.

## 3. Reproduction

```bash
cmake --preset debug
cmake --build --preset debug --parallel 4
ctest --preset debug
```

For a focused audit:

```bash
ctest --test-dir build/debug --output-on-failure -R 'legacy-parity|nucleon-distributions'
```

The full suite currently registers nine tests when both workflows are enabled and csh/tcsh is available (eight without the shell). Temporary directories isolate generated fixtures and output. Successful test output is the current executable evidence; rerun after changing the reference sources or production algorithms.

## 4. Matrix and criteria

| Test | Cases | Pass criterion |
| --- | --- | --- |
| `uniform-integration` | 1e/ep/en, seed repeat, tester, config overrides, invalid inputs | Output counts/PDGs, bounds, mass-shell relation in precise mode, unchanged repeat output and rejection semantics |
| `genie-integration` | Processes/species, partial files, capacity limit, missing/wrong branches, empty/unsupported input, 300-particle arrays, broken later chain file | Correct records/counts and no completed manifest on failures |
| `simulation-integration` | Dry runs, index selection, count propagation, successful stubs, failing GEMC | No dry-run writes, correct argument counts, reconstruction skipped on failure, locks preserved |
| `uniform-legacy-parity` | 1e/ep/en/tester at 2.07052/4.02962/5.98636 GeV; two files each; additional target geometries | Exact LUND bytes; original channel histograms match numerically |
| `genie-legacy-parity` | 10000 accepted events at each legacy energy and associated C12 geometry; all retained species/processes; short fixture | Exact LUND bytes/full-file diagnostics; explicitly confirmed short-input correction |
| `nucleon-distributions` | 20000 sampled en and 20000 sampled ep events | Bounds and empirical-CDF distance <0.025 from expected phi, angular and momentum distributions |
| `submission-legacy-parity` | Uniform and physical Bash payloads at 2/4/6 GeV | Exact argv after normalizing temporary run-directory paths; 10000-event entries retain legacy arguments |

Uniform reference seeds are kinematic 67890 and vertex 12345. Target checks cover Ar plus liquid, 4-foil, 1-foil, 1-foil-small, 1-foil-large and Ca; the tester uses a point vertex. Photon and all pion/nucleon species are included in the GENIE fixture. Comparisons use the restored archived pion constants, including π⁰=0.13957 GeV.

CDF tests independently evaluate the formulas in [sampling models](sampling-models.md). The ep test checks the uniform-p subsequence, uniform-1/p subsequence and the combined mixture; the en test checks cosθ and p. A fixed numerical threshold is used as a regression criterion, not as a formal significance claim across arbitrary seeds.

The `replacement-geometry` test verifies header-only geometry updates, new target names and RNG independence. The `ssh-launcher` test verifies sourced-shell survival/status, argument quoting, settings, build invocation and safe updates against a local Git fixture. Neither requires a remote server.

## 5. Intentional differences and limits

- **Short GENIE input:** the archived loop stops after one accepted event when fewer than 10000 entries remain. In the seven-entry fixture, the old converter writes one event while the new converter writes all six accepted events. The correction is retained; parity is not claimed for this defect or for its near-end truncation behavior.
- **Unknown historical RNG state:** archived `TRandom3(0)` runs cannot be reconstructed from a seed that was never recorded. Deterministic parity uses an explicitly substituted nonzero seed.
- **Precise format or standard masses:** these explicitly change output and are outside legacy-byte equivalence.
- **New sampled modes:** these follow the requested neutron/proton distributions and are not equal to the fixed legacy output.
- **Diagnostics and paths:** histogram data are restored; file names/plot styling are standardized, and user-selected run directories differ. The new manifest adds provenance.
- **Metadata:** ready-made Ar examples use A=40/Z=18, while the original uniform launcher used A=Z=1. Match settings explicitly; `legacy-coderun.conf` captures the archived values.
- **Detector processing:** local executable stubs verify command contracts only. GEMC/server software, geometry databases, reconstruction versions, field settings and simulation RNG state are required before claiming identical HIPO or acceptance results.

## 6. Server validation for a technical note

Use a small matched legacy/current LUND sample, identical GEMC/reconstruction versions, identical card/YAML/database resources and explicit detector RNG control if supported by the production setup. Compare event counts, generated banks, reconstructed particle yields and acceptance distributions, retaining logs and resource hashes. Document tolerances and statistical uncertainties. No server jobs are submitted by the repository's tests.

The simulation integration test also checks external-payload monitoring, its recorded hash, installed discovery, a non-GENIE generator and explicit rejection of partial files or incompatible settings. Submission parity compares detector argv to both protected originals at all three beam energies, including their monitoring labels. No real jobs are submitted.

The unified payload is checked against the legacy GENIE script after normalizing its single generalized event-count assignment back to `NEVENTS=10000`. For a 10000-event manifest entry, detector and reconstruction argv remain identical. Coordinator integration separately verifies that shorter manifest entries reach both commands with their exact count.
