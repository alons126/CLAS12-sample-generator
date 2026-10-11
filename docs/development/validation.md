# Scientific validation boundaries

Two different checks are needed: does the code do what its documentation says, and does the resulting simulation suit the intended physics analysis?

Software checks cover accepted options, event selection, sampling rules, record fields, units, particle order, file splitting, logs, safe deletion paths, and job submission. For example, they can show that a LUND file contains the requested particles and uses the configured beam energy. They do not show whether the detector model matches data.

For example, a study of electron detection at one beam energy may use several simulation runs with the same detector settings. That related set of runs is a **simulation campaign**. A **data campaign** is a related set of detector data-taking runs; a **GENIE production campaign** is a related set of upstream event-generator runs.

Before using results in an analysis, check the complete simulation campaign. Use the intended GEMC and COATJAVA versions, GCARD and YAML files, external databases, detector random settings, target code, input dataset, and reconstruction conditions. Compare output distributions with enough statistics to judge whether differences are meaningful.

These comparisons are **detector-level validation**: they check simulated and reconstructed results under specified detector and reconstruction conditions, with enough statistics to assess their suitability for the intended analysis. Passing software checks alone does not provide this evidence.

## Important software boundaries

- Uniform samples are acceptance probes, not physical interactions.
- A physical-input adapter copies supported truth and does not invent missing kinematics.
- [`TRandom3(0)`](https://root.cern.ch/doc/master/classTRandom3.html) is not reproducible from the recorded zero.
- Target geometry and LUND $A$ / $Z$ metadata are independently configurable.
- The GENIE process code in LUND field 10 is not a cross-section weight.
- Physical file cutoff counts remaining input entries, not remaining accepted events.
- Uniform monitoring checks generated distributions; it is not reconstructed acceptance.
- An accepted Slurm job ID does not show that any task completed.
- One readable HIPO file does not show that every task succeeded or that detector output is equivalent.

## Current uniform status

The electron-tester, 1e, epFD, and enFD modes are the production-tested uniform modes. Here, **production-tested** means reported as used and checked in detector-simulation production; it does not certify every configuration. FD charged-pion modes and all CD modes are implemented but remain unvalidated for production. Keep that warning in their profiles and user documentation until detector-level evidence supports changing it.

This manual does not include a [campaign record](#campaign-record) linking that production-tested designation to an exact source revision, detector configuration, and comparison result. Treat it as the reported status of those modes, not certification of every target, beam, release, or override. Before reusing a result in an analysis, obtain the corresponding campaign record from the collaboration member responsible for that production.

## Campaign record

For that electron study, collect the run settings, job logs, comparison plots, and conclusions in a report with links to the supporting files. This report and its supporting files form the **campaign record**. The person or collaboration group responsible for the campaign assembles it so another reader can assess whether the results suit an analysis. It has no required file format and is not generated automatically by this code.

The [LUND creation log](../concepts/lund-data-contract.md#lund-creation-log) and [Slurm submission log](../concepts/lund-data-contract.md#slurm-submission-log) are automatically written JSON files used as inputs to the campaign record. They record software stages and accepted submissions; they do not replace detector-level comparisons or scientific review.

Retain the source revision, LUND creation log, original physical input, ROOT/compiler versions, GEMC and COATJAVA versions, loaded modules, target-header hash, GCARD/YAML hashes, geometry and calibration database versions, detector random settings, scheduler logs, expected and actual file inventories, comparison tolerances, and statistical uncertainties.

For acceptance work, compare generated and reconstructed counts and distributions using matched samples and identical detector/reconstruction conditions. Preserve the ROOT histograms behind exported figures. Local creation monitoring cannot substitute for these detector-level comparisons.

A reviewable campaign record should identify the person or collaboration group responsible, date, sample modes and targets, exact source and input revisions, the settings listed above, comparison plots, numerical acceptance criteria, observed results, and approval or remaining limitations. Link that record from this page when it is available; do not replace missing evidence with a successful build or a readable HIPO file.
