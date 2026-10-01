# Adding a physical-input adapter

Add another generator/format reader behind `event-generator-to-lund-converter`. Do not create a new top-level workflow or copy common target, writer, naming, splitting, provenance, or completion code.

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

Create a format-specific sibling:

```text
src/workflows/lund-creation/event-generator-to-lund-converter/
└── mygenerator-myformat/
    ├── MyGeneratorConverter.h
    └── MyGeneratorConverter.cpp
```

Expose one synchronous function that borrows the resolved `RunConfig` and either completes the run or throws. Use an adapter name that includes the format when a generator has several outputs.

## 3. Validate before reading indexed data

Check files, record containers, required fields, stored types, array lengths, units, and cross-field consistency before indexed access. Stop instead of guessing malformed input. Validate later files in a chain as well as the first.

If no supported event is written, fail without publishing a completion manifest.

## 4. Translate to the common model

For each accepted input event:

1. fill one `Event` with documented header metadata;
2. sample exactly one vertex through `TargetGeometry`;
3. add supported particles in the documented order;
4. use `getParticleMass()` for supported project species;
5. give every particle the same vertex; and
6. call `LundWriter::writeEvent()`.

The adapter owns input traversal and scanned/written accounting. `LundWriter` owns accepted-event capacity, filenames, rollover, serialization, guarded output replacement, and the final manifest. Physical adapters do not create uniform monitoring products.

## 5. Register settings and dispatch

Add the adapter identifier to the physical branch of `RunConfig`. Add only settings that are genuinely format-specific; keep input, target, output, capacity, splitting, and provenance in the common physical contract.

Add one explicit branch in `convertPhysical()`. The current supported set is small, so a direct dispatcher is clearer than a speculative plugin lifecycle.

Update CLI help and add a checked-in example profile with explicit metadata. Preserve the public executable name and `--event-generator` interface.

## 6. Add build integration

Create a format-specific library and link only its parser/runtime dependencies. Keep those dependencies out of `LundCore` and the uniform-only build. If a new optional build switch can remove the adapter, ensure the dispatcher cannot reference a library that was not built.

## 7. Preserve names and provenance

Physical output names include target, adapter, optional generator version, tune or model label, input-selection label, and beam energy. Use explicit `none` or `unknown` when a field does not apply. Keep every unsanitized resolved value separately in the manifest. GEMC and COATJAVA versions remain later submission choices.

## 8. Validate the boundary

Exercise every retained process and particle, ordering, shared vertices, malformed records, skipped content, unsupported-only input, capacity, rollover, short input, exact block boundaries, later-file read failures, counts, provenance, and failure without a manifest. Confirm that physical conversion produces no uniform monitoring products.

Update the physical user guide, configuration reference, data contract, source map, tutorials, and scientific validation status in the same change.
