# Contributing

Keep each change focused. Make sure the code, help, profiles, and documentation all describe the same behavior.

## Before changing code

Read the [architecture](../concepts/architecture.md), the user guide for the affected workflow, and the [validation boundaries](validation.md). Discuss a new workflow, scientific convention, file format, or output contract before implementing it; those choices affect collaborators and existing campaign data.

Do not casually edit protected external inputs:

- [`src/workflows/lund-creation/external/targets.h`](../../src/workflows/lund-creation/external/targets.h)
- [`src/workflows/slurm-submission/external/submit_GEMC_sample.sh`](../../src/workflows/slurm-submission/external/submit_GEMC_sample.sh)
- anything under [`config/detector/`](../../config/detector)

The [external-input guide](../concepts/external-inputs.md) defines their ownership and replacement process.

## Implement within the existing boundaries

- Keep LUND configuration in `RunConfig`, source-specific event logic in its producer or adapter, and serialization/completion in `LundWriter`.
- Keep uniform monitoring with the uniform LUND creator.
- Keep submission resolution separate from environment loading, output actions, and the external task worker.
- Add a new user-facing workflow as a peer under [`src/workflows/`](../../src/workflows) only when implementation begins.
- Move code into shared support only when more than one implemented workflow needs the same real contract.

Use the terms **uniform LUND creator** and **physical LUND converter** when the distinction matters. The latter converts existing truth and does not run an event generator.

## Validate the change

Configure a fresh development build, compile the affected targets, inspect current `--help`, and run the smallest meaningful example. For submission changes, use preview first and verify that resolved settings and planned actions are correct. For format or output changes, inspect the generated records and manifest rather than relying only on process exit status.

A successful build shows that the code compiles and links. It does not show that GEMC and reconstruction produce scientifically correct results. Record the production checks still needed and follow the [scientific validation guide](validation.md).

## Update the complete contract

In the same change, update every affected item:

- source comments and public interfaces;
- CLI help and validation errors;
- checked-in profiles and configuration references;
- user procedures, data contracts, and failure behavior;
- tutorials that demonstrate the changed option or command; and
- the Wiki source.

Do not add build trees, generated samples, HIPO files, credentials, personal paths, or farm logs to Git. Keep historical comparisons and publication working notes outside the reader documentation.

## Check the Wiki

Build a local generated copy with [`build_wiki.py`](../../dev-tools/wiki/build_wiki.py) from the repository root:

```bash
wiki_preview="$(mktemp -d)"
python3 dev-tools/wiki/build_wiki.py \
    --output "$wiki_preview" \
    --repository ORGANIZATION/REPOSITORY \
    --branch main
```

Replace the repository placeholder with the current fork. Generation must finish without broken local links, citation errors, missing pages, or page-name collisions. Do not edit generated Wiki pages directly.
