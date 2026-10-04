# Scientific validation boundaries

Two different checks are needed: does the code do what its documentation says, and does the resulting simulation suit the intended physics analysis?

Software checks cover accepted options, event selection, sampling rules, record fields, units, particle order, file splitting, logs, safe deletion paths, and job submission. For example, they can show that a LUND file contains the requested particles and uses the configured beam energy. They do not show whether the detector model matches data.

Before using results in an analysis, check the complete simulation campaign. Use the intended GEMC and COATJAVA versions, GCARD and YAML files, external databases, detector random settings, target code, input dataset, and reconstruction conditions. Compare output distributions with enough statistics to judge whether differences are meaningful.

## Important software boundaries

- Uniform samples are acceptance probes, not physical interactions.
- A physical adapter copies supported truth and does not invent missing kinematics.
- [`TRandom3(0)`](https://root.cern.ch/doc/master/classTRandom3.html) is not reproducible from the recorded zero.
- Target geometry and LUND $A$ / $Z$ metadata are independently configurable.
- The GENIE process code in LUND field 10 is not a cross-section weight.
- Physical file cutoff counts remaining input entries, not remaining accepted events.
- Uniform monitoring checks generated distributions; it is not reconstructed acceptance.
- An accepted Slurm job ID does not show that any task completed.
- One readable HIPO file does not show that every task succeeded or that detector output is equivalent.

## Current uniform status

The electron-tester, 1e, epFD, and enFD modes are the production-tested uniform modes. FD charged-pion modes and all CD modes are implemented but remain unvalidated for production. Keep that warning in their profiles and user documentation until detector-level evidence supports changing it.

## Campaign record

Retain the source revision, completion manifest, original physical input, ROOT/compiler versions, GEMC and COATJAVA versions, loaded modules, target-header hash, GCARD/YAML hashes, geometry and calibration database versions, detector random settings, scheduler logs, expected and actual file inventories, comparison tolerances, and statistical uncertainties.

For acceptance work, compare generated and reconstructed counts and distributions using matched samples and identical detector/reconstruction conditions. Preserve the ROOT histograms behind exported figures. Local creation monitoring cannot substitute for these detector-level comparisons.
