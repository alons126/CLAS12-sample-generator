# Public GitHub Wiki

The repository is public on GitHub. Its GitHub Wiki is a generated reading view of the maintained Markdown in this checkout; repository files remain the source of truth.

## Publication workflow

[`scripts/build_wiki.py`](../../scripts/build_wiki.py) collects `README.md`, every `docs/**/*.md` page, the tutorial index, the sample-profile reference, and the launcher/build reference pages. It creates collision-free page names in GitHub Wiki's flat namespace, maps `docs/index.md` to `Home.md`, groups links by subject in `_Sidebar.md`, generates `_Footer.md`, rewrites documentation links to wiki pages, and rewrites links to code or configuration as public GitHub source URLs.

The [Publish documentation wiki](../../.github/workflows/publish-wiki.yml) action runs after matching documentation changes reach `dev` or `main`. It builds into a temporary directory, checks out the separate `<repository>.wiki.git` repository, synchronizes the generated tree, and pushes only when content changed. Generated source links point to the branch that triggered publication, so the public wiki can follow active development before the project is ready to merge into `main`.

Direct wiki edits are intentionally temporary because the next successful publication replaces them. Make maintained edits in this repository and submit them through its normal review history.

## Initial GitHub setup

1. Enable **Wikis** under the public repository's **Settings → Features**.
2. Create and save the initial `Home` page so GitHub initializes the separate wiki Git repository.
3. Push a matching documentation change to `dev` or `main`. Manual runs become available when GitHub recognizes the workflow on the default branch, but they are not required for `dev` publication.

The action first uses its repository-scoped `GITHUB_TOKEN`. If repository policy does not permit that token to push the wiki Git repository, create a repository secret named `WIKI_TOKEN` containing a narrowly scoped token with permission to write this repository; the workflow automatically prefers that secret when present.

## Local preview

From the repository root:

```bash
wiki_preview="$(mktemp -d)"
python3 scripts/build_wiki.py \
  --output "$wiki_preview" \
  --repository alons126/CLAS12-sample-generator \
  --branch main
```

The command prints the generated page count. Inspect the temporary directory as Markdown, then remove it when it is no longer needed. Generation rejects a destination that is the checkout itself or one of its ancestors, unresolved local links, missing required source pages, and colliding wiki filenames.
