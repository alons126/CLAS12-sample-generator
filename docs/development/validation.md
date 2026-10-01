# Scientific validation boundaries

Software validation and detector-level scientific validation answer different questions.

Software validation establishes that the implementation follows its documented contract: accepted options, event selection, sampling rules, record fields, units, ordering, file splitting, manifests, path safety, and submission handoff. It can show that a file is readable and that configured values were propagated correctly.

Production validation must also establish that the complete campaign is suitable for its analysis. That requires the intended GEMC and COATJAVA versions, GCARD and YAML resources, external databases, detector random state, target implementation, input dataset, reconstruction conditions, and statistically meaningful comparisons.

## Important software boundaries

- Uniform samples are acceptance probes, not physical interactions.
- A physical adapter copies supported truth and does not invent missing kinematics.
- `TRandom3(0)` is not reproducible from the recorded zero.
- Target geometry and LUND $A$/$Z$ metadata are independently configurable.
- The GENIE process code in LUND field 10 is not a cross-section weight.
- Physical file cutoff counts remaining input entries, not remaining accepted events.
- Uniform monitoring checks generated distributions; it is not reconstructed acceptance.
- An accepted Slurm job ID does not show that any task completed.
- One readable HIPO file does not show that every task succeeded or that detector output is equivalent.

## Current uniform status

The 1e, epFD, enFD, and electron-tester modes are the production-tested uniform modes. FD charged-pion modes and all CD modes are implemented but remain unvalidated for production. Keep that warning in their profiles and user documentation until detector-level evidence supports changing it.

## Campaign record

Retain the source revision, completion manifest, original physical input, ROOT/compiler versions, GEMC and COATJAVA versions, loaded modules, target-header hash, GCARD/YAML hashes, geometry and calibration database versions, detector random settings, scheduler logs, expected and actual file inventories, comparison tolerances, and statistical uncertainties.

For acceptance work, compare generated and reconstructed counts and distributions using matched samples and identical detector/reconstruction conditions. Preserve the ROOT histograms behind exported figures. Local creation monitoring cannot substitute for these detector-level comparisons.
