# Publishing the GitHub Wiki

Edit the repository's Markdown files to change the manual. The publication workflow copies those files into the GitHub Wiki, where readers can browse them. There is only one set of editable documentation, not separate repository and Wiki manuals. [`README.md`](../../README.md) introduces the repository, and [`docs/index.md`](../index.md) becomes the Wiki home page.

Never edit a generated Wiki page directly. Change the repository source and let publication replace the generated Wiki.

## What is published

[`dev-tools/wiki/build_wiki.py`](../../dev-tools/wiki/build_wiki.py) collects:

- [`README.md`](../../README.md);
- every non-ignored Markdown page under [`docs/`](../), matching `docs/**/*.md`;
- [`tutorials/README.md`](../../tutorials/README.md);
- [`config/samples/README.md`](../../config/samples/README.md); and
- [`config/run.json.md`](../../config/run.json.md).

For example, [`docs/getting-started/quickstart.md`](../getting-started/quickstart.md) becomes the Wiki page `getting-started-quickstart`. The builder gives each source a unique page name because Wiki pages do not use the repository's directory structure. It also creates the sidebar and footer, changes documentation links to Wiki links, changes code and configuration links to repository URLs, and converts footnotes to numbered references.

Each sidebar section starts with its overview, then follows the local page links in that section's [`index.md`](index.md). Unlisted pages appear afterward in Wiki-name order. Update the section index when changing its reading order; there is no separate sidebar order to maintain.

Git-ignored local drafts and copies are skipped. Development-only publication notes, bibliography material, and historical comparisons are not Wiki inputs.

## Local build

```bash
wiki_preview="$(mktemp -d)"
python3 dev-tools/wiki/build_wiki.py \
    --output "$wiki_preview" \
    --repository ORGANIZATION/REPOSITORY \
    --branch main
```

Use the current fork's `OWNER/NAME`. The Wiki builder rejects unsafe output locations, broken local links, missing mapped pages, page-name collisions, and invalid or unused footnotes.

## GitHub publication

The `Publish documentation wiki` action checks documentation-related pull requests. When files covered by the action's filters reach `dev` or `main`, it builds the Wiki from that repository and branch, copies the generated pages into GitHub's separate `<repository>.wiki.git` repository, and pushes them if they changed. Links use the repository running the action, so a fork does not need to replace a hard-coded repository URL.

There is one Wiki per repository, not one per branch. Publications from `dev` and `main` replace the same pages; the last successful publication determines which branch's documentation and source links readers see. A code-only push does not trigger this workflow. If source changes move automatically linked function definitions without changing documentation, run the action manually to refresh their line links.

GitHub creates the separate Wiki Git repository only after the repository Wiki has been enabled and one initial page has been saved. This is a one-time repository setup step:

1. Enable **Wikis** under **Settings → Features**.
2. Save one initial Wiki page so `<repository>.wiki.git` exists.
3. Push a matching documentation change to `dev` or `main`; the workflow replaces that initial page with generated content.

After this one-time setup, collaborators edit only the repository Markdown files. If the normal `GITHUB_TOKEN` cannot push to the Wiki repository, add a repository secret named `WIKI_TOKEN` with the required access. The workflow uses that token when present.

## Link conventions

Use repository-relative Markdown links for documents and explicit repository paths for source. Inline-code file paths and unambiguous qualified function names can be linked automatically. Keep command examples in fenced blocks so they remain copyable. When a short filename is ambiguous, write the full repository-relative path.
