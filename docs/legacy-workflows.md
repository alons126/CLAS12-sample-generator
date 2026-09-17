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

`CodeRun.cpp` selects channels through three booleans and currently makes three active calls: **1e, ep and en at 2.07052 GeV**, each under a GEMC 5.14 `rgm_fall2021_C_S` output label. Each call inherits 5000 files × 10000 events. The generator itself hardcodes `1-foil-small` vertex geometry and A=Z=1 header metadata. Its local kinematic `TRandom3(0)` is automatically seeded; the target helper uses a global seed of 12345. Consequently, the exact historical production event sequence cannot be reconstructed unless the automatically chosen kinematic seed/state was recorded.

The three active `CodeRun.cpp` calls map to these maintained commands. `legacy-coderun.conf` already supplies 50,000,000 total events, 10,000 events per file, `1-foil-small` geometry, A=Z=1, legacy masses/formatting, and the archived angular ranges. `--seed 0` preserves the archived request for ROOT automatic seeding, although it cannot reproduce an earlier automatically seeded sequence.

```bash
source run.csh --workflow create-lund --source uniform \
  --config config/samples/legacy-coderun.conf \
  --seed 0 \
  --output OUTPUT_PARENT

source run.csh --workflow create-lund --source uniform \
  --config config/samples/legacy-coderun.conf \
  --channel eh --hadron proton --hadron-region FD \
  --electron-momentum beam \
  --hadron-momentum uniform --hadron-angle theta \
  --hadron-p-min 0.3 --hadron-p-max 2.07052 \
  --prefix Uniform_ep_sample_2070MeV \
  --seed 0 \
  --output OUTPUT_PARENT

source run.csh --workflow create-lund --source uniform \
  --config config/samples/legacy-coderun.conf \
  --channel eh --hadron neutron --hadron-region FD \
  --electron-momentum beam \
  --hadron-momentum uniform --hadron-angle theta \
  --hadron-p-min 0.3 --hadron-p-max 2.07052 \
  --prefix Uniform_en_sample_2070MeV \
  --seed 0 \
  --output OUTPUT_PARENT
```

The three commands can share one output parent because the maintained writer creates distinct `Uniform_sample_1e_2070MeV/`, `Uniform_sample_epFD_2070MeV/`, and `Uniform_sample_enFD_2070MeV/` run directories below it. The old launcher instead rewrote `OutPut/` to sibling `OutPut_1e/`, `OutPut_ep/`, and `OutPut_en/` directories. The LUND filename prefixes above retain the archived names. Use a smaller explicit `--events` value for a smoke test.

The commented 4.02962 and 5.98636 GeV calls use the same commands with `--beam-energy 4.02962` or `5.98636`, matching `--hadron-p-max`, and prefixes ending in `4029MeV` or `5986MeV`. Automatic trigger offsets resolve to the same legacy 7° and 5° prescriptions; the active 2.07052 GeV commands resolve to 16°.

The separately selectable legacy electron tester maps to:

```bash
source run.csh --workflow create-lund --source uniform \
  --config config/samples/electron-tester-2070.conf \
  --beam-energy 2.07052 \
  --A 1 --Z 1 \
  --events 1000000 --events-per-file 10000 \
  --prefix Uniform_1e_sample_2070MeV \
  --seed 0 --mass-convention legacy --lund-format legacy \
  --output OUTPUT_PARENT/tester
```

One million events reproduces the tester's server default of 100 files × 10,000 events. Its old local-path branch reduced that to 100,000 events. Repeat with the two other beam energies and matching prefixes for the commented tester calls. The tester profile supplies its fixed `(0,0,-3 cm)` vertex and beam-momentum electron.

The pinned upstream ep/en prescription maps to uniform hadron momentum, flat theta, and a 0.3 GeV/c lower bound. `fixed` preserves the older selectable 1 GeV/c neutron mode, while the maintained production profiles activate the newer channel-dependent prescriptions.

The reference tests call the actual pinned upstream event functions. They supply deterministic seeds, initialize upstream histograms and write into temporary directories; they do not source `run.sh`, which contains repository cleanup/update commands. With matched modes, LUND bytes and numerical histogram contents agree for 1e, ep, en and the tester at all three established beam energies.

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
build/debug/apps/clas12-generator-to-lund --event-generator genie \
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
| `NEVENTS=10000` in archived payloads | `JOB_NEVENTS` from each validated manifest entry; old argument parity holds when that entry contains 10000 events |
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

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
