# Migration from the imported repositories

The old `Uniform-sample-generator/`, `GEMC-samples/`, and root GENIE submission script are archived under `legacy/`. Their source is retained for comparison, excluded from the root build, and not supported as runnable entry points. Historical scripts include hardcoded site paths and destructive cleanup; do not run them. Use the compiled applications and new runner.

## Entry points

| Previous | Supported replacement |
| --- | --- |
| `Uniform-sample-generator/CMakeLists.txt` | Root `CMakeLists.txt` and presets |
| ROOT `CodeRun.cpp` with edited production calls | `clas12-uniform --config FILE --output NEW_DIR` |
| `Uniform_sample_generator_e_tester.C` | `electron-tester.conf` (beam momentum, point vertex) |
| `GENIE_to_LUND_converter.csh` / ROOT macro | `clas12-genie-to-lund --input ... --config ... --output ...` |
| Per-energy/channel GEMC submission scripts | `scripts/simulation/run.py` and `scripts/slurm/submit.py` |
| `Generation_files_*` | `config/detector/Generation_files_*` |
| Current-directory-based output rewrites | Explicit `--output` |

The attachment-inspired `src/` library and `apps/` executable split is now the root build. Shared libraries here mean reusable code targets; the default implementation links them statically into the applications.

## Preserved prescriptions

Electron uniform sampling is flat in theta, phi and momentum. Electron–nucleon modes retain the fixed 1 GeV nucleon default, shared vertex, 25° trigger theta and opposite-sector trigger phi. The two imported target maps have been consolidated with their original positions and transverse spread. GENIE conversion retains electron-first ordering, supported particle species, process priority and resonance/process header conventions.

## Deliberate changes

- Kinematic and vertex seeds are explicit, nonzero and recorded. Historical random sequences are not guaranteed.
- LUND output uses ten significant digits and run-global uniform event IDs instead of resetting IDs in each file.
- Target geometry and A/Z are separate explicit settings. Ar example configs use A=40/Z=18; the bare CLI retains A=Z=1 defaults.
- GENIE filenames no longer supply energy/target/tune assumptions. The final partial file is kept, and the file limit applies to accepted events.
- GENIE energies are recomputed from shared masses and momenta. Electron/proton/neutron constants match the imported uniform code; pion constants are explicit (charged 0.13957039, neutral 0.1349768 GeV). Review mass conventions against historical external utilities when comparing physical samples.
- Diagnostics are a per-PDG ROOT file with consistent names. The old global histogram objects, electron–nucleon correlation collection, PDF/PNG styling and ROOT list container are archived, not part of the new output contract. Downstream scripts reading those diagnostic names need updating; LUND/GEMC remain the pipeline interface.
- Simulation uses actual manifest counts instead of an independent hardcoded 10,000-event limit.
- Output creation never deletes existing directories. Failures return nonzero, and manifests are published only on successful completion.
- The normal build/run workflow contains no Git update or reset operations.

Use small samples to compare distributions before regenerating production acceptance inputs. Automated checks cover software behavior, not equivalence of full reconstructed acceptance maps.
