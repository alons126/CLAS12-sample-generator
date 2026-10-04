# Contributing

The complete contribution guide is in the generated Wiki source at [`docs/development/contributing.md`](docs/development/contributing.md). Follow the [developer reading order](docs/development/index.md) and read the user guide for the workflow you intend to change.

When changing code, update its help, profiles, tutorials, and documentation in the same change, then check that they agree. Do not edit generated Wiki pages directly. Edit the repository Markdown files; the publication workflow rebuilds the Wiki from them.

To prepare a release while keeping development files on `dev`, follow [the publication PR guide](docs/development/release-publication.md). Its manually triggered action validates a filtered snapshot before updating `release-candidate` and opening a PR into `main`.
