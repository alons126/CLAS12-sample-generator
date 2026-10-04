# Development guide

Read these pages in this order when changing the project:

1. [Architecture](../concepts/architecture.md)
2. [Source and API map](source-reference.md)
3. [LUND data contract and provenance](../concepts/lund-data-contract.md)
4. [Sampling models and random numbers](../concepts/sampling-models.md)
5. [External geometry and detector inputs](../concepts/external-inputs.md)
6. [Contributing](contributing.md) and [source documentation conventions](documentation-style.md)
7. [Developer build reference](building.md)
8. [Scientific validation boundaries](validation.md)

Then use [adding a physical-input adapter](adding-event-generator.md) for that specific extension, or [Wiki publishing](wiki-publishing.md) when changing documentation and its publication. The [getting-started path](../getting-started/index.md) remains the operating guide; developer pages explain the implementation behind it.

Code, CLI help, profiles, tutorials, and the Wiki describe one system. A change is incomplete while any affected layer still describes the previous behavior.
