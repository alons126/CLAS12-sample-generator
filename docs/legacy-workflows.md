# Legacy launch chains and current equivalents

## 1. Uniform generation

The archived entry point was sourced in a csh/tcsh environment:

```text
source Uniform-sample-generator/run.sh
  -> root ./CodeRun.cpp -l -q -b
     -> manually selected Uniform_sample_generator(...) call
        -> Generate_uniform_event(...) overload for 1e or ep/en
        -> LUND + monitoring products
```

`CodeRun.cpp` selects the channel through three booleans and passes beam energy, output directory and counts. Its currently active archived call is **1e, 2.07052 GeV, Ar output label, 4000 files × 25000 events**. The generator itself hardcodes Ar vertex geometry and A=Z=1 header metadata. Its local kinematic `TRandom3(0)` is automatically seeded; the target helper uses a global seed of 12345.

Current equivalent with small counts:

```bash
build/debug/apps/clas12-uniform \
  --config config/samples/legacy-coderun.conf \
  --files 1 --events-per-file 100 \
  --output runs/legacy-coderun-smoke
```

Select `--channel ep` or `--channel en` and a matching prefix instead of editing C++ calls. `--nucleon-momentum fixed` preserves the archived 1 GeV mode. `sampled` activates the new requested channel-dependent prescription. `electron-tester.conf` replaces the separately selected tester call.

The reference tests call the actual archived event functions. They supply deterministic seeds, initialize archived histograms and write into temporary directories; they do not source `run.sh`, which contains repository cleanup/update commands.

## 2. GENIE conversion

```text
GENIE_to_LUND_converter.csh
  -> source scripts/set_env.csh
  -> select nucleus, tune, energy, counts and input tree directory
  -> map nucleus/energy to A, Z and vertex geometry
  -> root GENIE_to_LUND_converter.C(input_glob, files, geometry, A, Z)
```

Active archived values are C12, tune `GEM21_11a_00_000`, energy label `2070MeV`, maximum 5000 files, A=12, Z=6 and geometry `1-foil-small`. The input convention was:

```text
BASE_TL_SAMPLE_DIR/NUCLEUS/TUNE/ENERGY_Q2LABEL/
    master-routine_validation_01-eScattering/*.root
```

| Nucleus / energy | Geometry | A/Z |
| --- | --- | --- |
| H1, any supported energy | liquid | 1/1 |
| D2, any supported energy | liquid | 2/1 |
| C12, 2.07052 GeV | 1-foil-small | 12/6 |
| C12, 4.02962 GeV | 1-foil-large | 12/6 |
| C12, 5.98636 GeV | 4-foil | 12/6 |
| Ar40, any supported energy | Ar | 40/18 |

The Q² labels were `Q2_0_02`, `Q2_0_25` and `Q2_0_40` at the three energies. These describe the selected upstream dataset/path; the converter does not apply those cuts.

```bash
build/debug/apps/clas12-genie-to-lund \
  --config config/samples/legacy-genie-wrapper.conf \
  --input '/shared/truth/C12/GEM21_11a_00_000/2070MeV_Q2_0_02/master-routine_validation_01-eScattering/*.root' \
  --files 1 --output runs/legacy-genie-smoke
```

The new CLI takes the input explicitly and checks the GST schema. It does not infer physical metadata from filenames. The archived converter reference used by tests keeps the original event loop; an adapter only redirects includes/output paths, captures its histogram and replaces directory shell calls with checked filesystem operations.

## 3. Shared detector-job submission

Both sample families used this chain, independently of how their LUND files were produced:

```text
source setup_and_submit_jobs.csh
  -> source scripts/set_env.csh
  -> source scripts/update_script.csh
  -> source run_setup_and_submission_scripts.csh
     -> manually uncomment uniform_setup_and_submit.csh
        OR genie_job_submission_script.csh
        -> select sample directories, GEMC module, target card and YAML
        -> sbatch array of submit_GEMC_uniform_sample.sh
           OR submit_GEMC_GENIE_sample.sh
           -> gemc -> recon-util
```

The current archived selection is uniform submission. Its active loop is **en at 2070MeV**, GEMC 5.14, target variation `rgm_fall2021_C_S`, using a `_ConstPn` output directory. This is different from the active 1e/Ar generation call in `CodeRun.cpp`. These files are snapshots of independently edited scripts; they are not a consistent single campaign. Select matching generation metadata and detector resources explicitly.

| Former setting/action | Current location/action |
| --- | --- |
| Uncomment uniform vs GENIE setup line | Choose generation/conversion CLI; both yield the same manifest contract |
| Hardcoded output/input prefixes | `--output`, `--prefix`, manifest file list |
| `NUM_OF_FILES` / Slurm array | Actual number of completed manifest files |
| `NEVENTS=10000` in payload | Per-file manifest count; old argument parity holds for full 10000-event files |
| `TARGET_VARIATION`, `GCARD_FILE` | Explicit `--gcard` path |
| `YAML_FILE` | Explicit `--reconstruction` path |
| `TORUS_FIELD` | `--torus 0.5` at 2 GeV, `--torus -1` at 4/6 GeV for the legacy setup |
| Solenoid −1.0 | Default `--solenoid -1` |
| `module load gemc/VERSION`, `GEMC_DATA_DIR` | Load matching server environment before running/submitting |
| Scheduler account/partition/time/memory/logs | Site JSON `slurm` fields |
| `mc_PREFIX_INDEX_torusFIELD.hipo` | Default `--output-naming legacy` |
| Git reset/cleanup and farm-output cleanup | No supported automatic equivalent |

Use `scripts/simulation/run.py` for a local preview/execution or `scripts/slurm/submit.py` for the corresponding array. Commands and detailed examples are in the [execution guide](gemc-reconstruction-batch-submission.md).

The new software does not log into the server, set up modules, or reproduce an unspecified detector RNG state. Those are necessary external conditions for detector-level reproducibility.

The supported checkout entry point is `source run.csh` in csh/tcsh; see [SSH execution](ssh-workflow.md). Geometry source, LUND format, gcard provenance and the required energy-dependent field settings are documented in [external inputs](external-inputs.md).

The [unified external GEMC payload](gemc-payload.md) documents `src/common/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
