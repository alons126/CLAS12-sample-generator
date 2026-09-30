# Create LUND files

`create-lund` prepares truth-level particle records for GEMC. It has two sources behind one common output contract:

| Source | Program | Use |
| --- | --- | --- |
| `uniform` | `uniform-lund-creator` | Sample deliberately unphysical momentum and angle ranges for detector-acceptance studies |
| `physical` | `event-generator-to-lund-converter` | Copy supported particles from existing physical event-generator truth |

The paths share configuration parsing, target geometry, event and particle records, LUND serialization, splitting, output naming, provenance, and completion behavior. They differ where the science differs: the uniform LUND creator generates kinematics and monitoring histograms; a physical adapter reads event content and must not invent missing kinematics.

## Creation lifecycle

1. Read built-in defaults, an optional `key = value` profile, and command-line overrides.
2. Resolve automatic values and validate the complete configuration.
3. Validate source-specific input before replacing output when possible.
4. Print the exact resolved run directory. If it exists, warn, remove that directory, and recreate it.
5. Write numbered LUND files and source-specific products.
6. Publish `lund-creation-log.json` last.

The manifest is the completion boundary. A failed run may leave partial files, but without the final manifest it is not ready for submission.

Start with the [uniform guide](uniform.md) or [physical guide](physical.md). Use the [configuration reference](configuration.md) for exact options, [examples](examples.md) for commands, and the [LUND data contract](../concepts/lund-data-contract.md) when reading or extending the writer.

Creating LUND files never submits detector simulation. That remains a separate [ifarm submission workflow](../submit-simulation/index.md).
