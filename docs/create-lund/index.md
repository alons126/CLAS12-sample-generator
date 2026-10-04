# Create LUND files

`create-lund` writes LUND text files containing particles before detector simulation. You choose where those particles come from with `--source`:

| Source | Program | Use |
| --- | --- | --- |
| `uniform` | `uniform-lund-creator` | Sample deliberately unphysical momentum and angle ranges for detector-acceptance studies |
| `physical` | `event-generator-to-lund-converter` | Copy supported particles from existing physical event-generator truth |

Both applications use the same code to read settings, sample vertex positions in the target, write LUND records, split files, name directories, and record the settings and counts. The uniform LUND creator randomly chooses momenta and angles and saves plots of them. The physical LUND converter reads those momenta from existing input; its format-specific reader, called an adapter, must not invent missing particle kinematics (momenta and angles).

## Creation lifecycle

1. Read built-in defaults, an optional `key = value` profile, and command-line overrides.
2. Replace `auto` settings with their calculated values and check all settings.
3. Check the input required by the selected source before deleting output where possible.
4. Print the complete run directory. If it exists, warn, delete it and everything inside it, and recreate it.
5. Write numbered LUND files; uniform creation also saves monitoring histograms and plots.
6. Write `lund-creation-log.json` last, listing the settings, files, and event counts.

That final JSON log is the completion manifest. Its presence marks successful creation. A failed run may leave partial LUND files; without the final manifest, do not submit them.

## Reading order

1. Choose the [uniform guide](uniform.md) or [physical guide](physical.md) for the source you need.
2. Use the [configuration reference](configuration.md) to choose settings and understand their defaults.
3. For uniform samples, read [monitoring](monitoring.md) to inspect the generated kinematics.
4. Use [examples](examples.md) for additional commands.

Read the [LUND data contract](../concepts/lund-data-contract.md) when inspecting records or extending the writer. Path placeholders follow the [output notation](../getting-started/outputs.md#path-notation).

Creating LUND files never submits detector simulation. That remains a separate [ifarm submission workflow](../submit-simulation/index.md).
