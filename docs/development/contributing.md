# Contributing

Contributions should leave the software, command-line help, checked-in profiles, and documentation describing one consistent workflow. Small, focused changes are easier to review and validate than unrelated changes combined in one branch.

## Before changing the project

1. Read the [architecture overview](../concepts/architecture.md) and the guide for the workflow you will change.
2. Check the [scientific validation boundaries](validation.md). A successful build verifies software integration, not detector-level physics.
3. Discuss a new workflow, file format, physics convention, or output contract with the project maintainers before implementing it.

Do not edit imported detector and geometry sources as part of routine project work:

- `src/workflows/lund-creation/external/targets.h` is the protected target-geometry and particle-mass source.
- `src/workflows/slurm-submission/external/submit_GEMC_sample.sh` is the imported worker payload adapted at a defined boundary.
- Files below `config/detector/` are externally supplied GCARD and reconstruction resources.

If one of these inputs must change, treat the update as a deliberate source replacement. Record its origin, compare behavior, update the relevant provenance documentation, and validate the affected production chain.

## Build and validate

Configure a fresh out-of-source build when changing compilers or ROOT installations:

```bash
cmake \
    -S . \
    -B build/debug \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug
cmake \
    --build build/debug \
    --parallel 4
```

Run the affected executable with `--help`, then exercise the changed path with a small event count or a submission preview. When the configured checkout provides validation targets, run:

```bash
ctest \
    --test-dir build/debug \
    --output-on-failure
```

Do not use a successful local run as evidence of detector-level acceptance equivalence. Production validation also requires the selected GEMC and reconstruction versions, detector resources, external databases, random-state controls, campaign inputs, and statistical comparisons described in the [validation guide](validation.md).

## Keep documentation synchronized

Update every affected explanation in the same change:

- user workflow pages for visible behavior, options, outputs, or failure modes;
- configuration references and sample profiles for setting changes;
- source contracts and comments for implementation behavior;
- examples whenever a command, path contract, or default changes.

Use the terms **uniform LUND creator** and **physical LUND converter** when distinguishing the two LUND sources. The physical LUND converter consumes existing event-generator truth; it does not run an event generator. Keep publication planning and historical comparisons under `tech-note/`; that development-only tree is outside the reader Wiki source.

Follow the [source documentation conventions](documentation-style.md) for C++, Python, shell, CMake, and configuration explanations.

## Verify the Wiki

The GitHub Wiki is the sole reader-facing long-form manual, and the repository Markdown is its source of truth. Never edit generated Wiki pages directly. Change the corresponding file under `docs/` or another mapped source, then build a local preview from the repository root:

```bash
wiki_preview="$(mktemp -d)"
python3 dev-tools/wiki/build_wiki.py \
    --output "$wiki_preview" \
    --repository ORGANIZATION/REPOSITORY \
    --branch main
```

Replace `ORGANIZATION/REPOSITORY` with the GitHub location of the current fork. GitHub Actions supplies this value automatically during publication. Generation must finish without unresolved local links, missing source pages, or page-name collisions. See [wiki publishing](wiki-publishing.md) for publication and initial GitHub setup.

## Keep commits reviewable

Do not commit build trees, generated samples, HIPO files, batch output, credentials, personal paths, or local tutorial copies. Explain the reason for the change, the visible behavior, and the validation performed in the proposed change description. Call out any scientific behavior that still needs production validation.
