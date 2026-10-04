# Adding a physical-input adapter

For example, to convert another event generator's output, add a reader that turns its events into the code's `Event` records. This reader is called a physical-input adapter. Select it through `event-generator-to-lund-converter`; do not add another user-facing workflow or copy the shared target, writer, naming, splitting, logging, or completion code.

## 1. Define the scientific input contract

Before coding, document:

- input format, required records, units, and ordering;
- supported interactions and particles;
- skipped content and the reason;
- event-ID and header-field meanings;
- treatment of weights, process codes, and generator metadata;
- upstream decay requirements;
- malformed-input and unsupported-only behavior; and
- whether a finite-input tail rule is needed.

A physical adapter copies available truth. It must not resample particle kinematics, manufacture missing decay products, or borrow GENIE-specific meanings without a scientific reason.

## 2. Add the adapter directory

Create a format-specific sibling under [`event-generator-to-lund-converter/`](../../src/workflows/lund-creation/event-generator-to-lund-converter/):

```text
src/workflows/lund-creation/event-generator-to-lund-converter/
└── mygenerator-myformat/
    ├── MyGeneratorConverter.h
    └── MyGeneratorConverter.cpp
```

Provide one function that reads the already-checked `RunConfig` without taking ownership of it. The function should return only after the run finishes, or throw an exception on failure. Include the input format in the adapter name when the event generator has several output formats.

## 3. Validate before reading indexed data

Check files, record containers, required fields, stored types, array lengths, units, and cross-field consistency before indexed access. Stop instead of guessing malformed input. Validate later files in a chain as well as the first.

If no supported event is written, fail without publishing a completion manifest.

## 4. Translate to the common model

For each accepted input event:

1. fill one `Event` with documented header metadata;
2. sample exactly one vertex through `TargetGeometry`;
3. add supported particles in the documented order;
4. use [`getParticleMass()`](../../src/workflows/lund-creation/core/lund/Particle.cpp#L33) for particle species supported by the code;
5. give every particle the same vertex; and
6. call [`LundWriter::writeEvent()`](../../src/workflows/lund-creation/core/lund/LundWriter.cpp#L102).

The adapter reads input in order and counts entries examined and events written. `LundWriter` enforces the output event limit, chooses filenames, starts new files, writes LUND text, checks paths before deleting output, and writes the final manifest. Physical adapters do not create uniform monitoring histograms or plots.

## 5. Register settings and dispatch

Add the adapter identifier to the physical branch of `RunConfig`. Add only settings that are genuinely format-specific; keep input, target, output, capacity, splitting, and provenance in the common physical contract.

Add a branch in [`convertPhysical()`](../../src/workflows/lund-creation/event-generator-to-lund-converter/PhysicalConverter.cpp#L41) that calls the new adapter when its name is selected. There are few adapters, so a direct function call is sufficient; do not add a plugin-loading system for this change.

Update CLI help and add a checked-in example profile with explicit metadata. Preserve the public executable name and `--event-generator` interface.

## 6. Add build integration

Create a format-specific library and link only its parser/runtime dependencies. Keep those dependencies out of `LundCore` and the uniform-only build. If a new optional build switch can remove the adapter, ensure the dispatcher cannot reference a library that was not built.

## 7. Preserve names and provenance

Physical output names include target, adapter, optional generator version, tune or model label, input-selection label, and beam energy. Use explicit `none` or `unknown` when a field does not apply. Keep every unsanitized resolved value separately in the manifest. GEMC and COATJAVA versions remain later submission choices.

## 8. Validate the boundary

Check every supported process and particle, particle order, and shared vertex. Also check malformed records, skipped particles, input with no supported events, the event limit, file splitting, short input, and input ending exactly at a file boundary. Test read failures in later files, event counts, recorded settings, and failures leaving no final manifest. Confirm that physical conversion produces no uniform monitoring histograms or plots.

Update the physical user guide, configuration reference, data contract, source map, tutorials, and scientific validation status in the same change.
