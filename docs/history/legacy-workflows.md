# Legacy launch chains and current equivalents

These archived call chains correspond to the legacy baseline recorded by the [`legacy-v1.0.0` GitHub release tag](https://github.com/alons126/CLAS12-sample-generator/releases/tag/legacy-v1.0.0). The Uniform source remains a pinned submodule, so the tag records its exact referenced revision rather than duplicating that independent repository.

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

The three active `CodeRun.cpp` calls map to these maintained commands. `legacy-coderun.conf` already supplies 50,000,000 total events, 10,000 events per file, `1-foil-small` geometry, A=Z=1, the target-source masses, established formatting, and the archived angular ranges. `--seed 0` preserves the archived request for ROOT automatic seeding, although it cannot reproduce an earlier automatically seeded sequence.

```bash
source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-lund-creation/legacy-coderun.conf \
  --seed 0 \
  --output OUTPUT_PARENT

source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-lund-creation/legacy-coderun.conf \
  --channel eh --hadron proton --hadron-region FD \
  --electron-momentum beam \
  --hadron-momentum uniform \
  --hadron-p-min 0.3 \
  --prefix Uniform_ep_sample_2070MeV \
  --seed 0 \
  --output OUTPUT_PARENT

source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-lund-creation/legacy-coderun.conf \
  --channel eh --hadron neutron --hadron-region FD \
  --electron-momentum beam \
  --hadron-momentum uniform \
  --hadron-p-min 0.3 \
  --prefix Uniform_en_sample_2070MeV \
  --seed 0 \
  --output OUTPUT_PARENT
```

The three commands can share one output parent because the maintained writer creates distinct `Uniform_sample_1e_2070MeV/`, `Uniform_sample_epFD_2070MeV/`, and `Uniform_sample_enFD_2070MeV/` run directories below it. The old launcher instead rewrote `OutPut/` to sibling `OutPut_1e/`, `OutPut_ep/`, and `OutPut_en/` directories. The LUND filename prefixes above retain the archived names. Use a smaller explicit `--events` value for a smoke test.

The commented 4.02962 and 5.98636 GeV calls use the same commands with `--beam-energy 4.02962` or `5.98636`, the hadron upper bound following beam energy automatically, and prefixes ending in `4029MeV` or `5986MeV`. Automatic trigger offsets resolve to the same legacy 7° and 5° prescriptions; the active 2.07052 GeV commands resolve to 16°.

The separately selectable legacy electron tester maps to:

```bash
source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-lund-creation/electron-tester-2070MeV.conf \
  --beam-energy 2.07052 \
  --A 1 --Z 1 \
  --events 1000000 --events-per-file 10000 \
  --prefix Uniform_1e_sample_2070MeV \
  --seed 0 \
  --output OUTPUT_PARENT/tester
```

One million events reproduces the tester's server default of 100 files × 10,000 events. Its old local-path branch reduced that to 100,000 events. Repeat with the two other beam energies and matching prefixes for the commented tester calls. The maintained tester keeps its beam-momentum electron and samples the selected target geometry.

The pinned upstream ep/en prescription maps to uniform hadron momentum, flat theta, and a 0.3 GeV/c lower bound. `fixed` preserves the older selectable 1 GeV/c neutron mode, while the maintained production profiles activate the newer channel-dependent prescriptions.

Historical comparisons require matched deterministic seeds, upstream histogram initialization, and isolated output directories. Do not source the archived `run.sh` for such a comparison because it contains repository cleanup and update commands. Account explicitly for the maintained mass source and tester-vertex behavior.

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
build/debug/apps/event-generator-to-lund-converter --event-generator genie-gst \
  --config config/samples/physical-lund-creation/legacy-genie-wrapper.conf \
  --input '/shared/truth/C12/GEM21_11a_00_000/2070MeV_Q2_0_02/master-routine_validation_01-eScattering/*.root' \
  --events 10000 --output runs/legacy-genie-smoke
```

The archived wrapper limited the number of input ROOT files. The maintained converter instead takes the complete input file or glob explicitly and limits accepted output with `--events`; it has no `--files` option. It checks the GST schema and does not infer physical metadata from filenames. Historical comparisons must preserve the archived event loop while isolating its output and replacing unsafe directory shell calls.

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
| `NEVENTS=10000` in archived payloads | `JOB_NEVENTS` defaults to the largest selected manifest count; explicit event-limit overrides are supported |
| `TARGET_VARIATION`, `GCARD_FILE`, `YAML_FILE` | Resolved manifest values and config/CLI overrides |
| `TORUS_FIELD` | Retained +0.5 at 2 GeV; −1.0 at 4/6 GeV |
| Solenoid −1.0 | Unchanged in the external payload |
| GEMC modules and `GEMC_DATA_DIR` | Preloaded on ifarm; checked by Python submission, with optional clas12Tags override |
| Scheduler resources and logs | Existing external payload directives |
| Simulation output reset | Recreate `mchipo`/`reconhipo` for either source; preserve LUND |
| Repository update | Guarded disposable-clone refresh in `run.csh` |

`source run.csh --workflow submit` sources the unified setup directly. Select completed samples with `--lund-dir`; supply optional config/CLI overrides. See the [submission guide](../submit-simulation/guide.md) for the maintained handoff. Server detector software and RNG state remain necessary external conditions for detector-level reproducibility.
