# Generated GitHub Wiki

The GitHub Wiki is the project's sole reader-facing long-form manual. The Markdown files in this checkout are its version-controlled source; they are not a second documentation product. `README.md` introduces the project and directs readers to the Wiki, while `docs/index.md` becomes the Wiki home page and defines its reading order.

Never edit a Wiki page directly. Make every correction in its repository source, review that change with the code, and let the publication workflow replace the generated Wiki. This rule prevents the published manual from drifting away from the files used during development and review.

## Publication workflow

[`dev-tools/wiki/build_wiki.py`](../../dev-tools/wiki/build_wiki.py) collects `README.md`, every `docs/**/*.md` page, the tutorial index, the sample-profile reference, and the launcher/build reference pages. Publication planning, bibliography material, and historical comparisons live under `tech-note/` and are not Wiki inputs. The generator creates collision-free page names in GitHub Wiki's flat namespace, maps `docs/index.md` to `Home.md`, groups links by subject in `_Sidebar.md`, generates `_Footer.md`, rewrites documentation links to wiki pages, and rewrites links to code or configuration as public GitHub source URLs.

GitHub renders footnotes in repository Markdown but does not support them in Wikis. The generator therefore converts each page's `[^key]` markers into linked superscript numbers and replaces its footnote definitions with a numbered **References** section. Citation numbers follow first use on each page, and the generated reference includes a return link to the first citation. Keep native footnote syntax in the repository source; do not hand-maintain a second Wiki-specific citation form.

Inline-code references in prose are linked automatically. An exact repository path, a source-relative path, or a unique shortened filename links to that file or directory on the publishing branch. A qualified function name or a uniquely defined function name links to the current definition line. Existing Markdown links are preserved, fenced command/code examples remain unchanged for copying, and ambiguous or external names remain plain code instead of linking to an arbitrary source location.

Write repository file references as Markdown links or inline code. Prefer repository-relative paths when a basename occurs more than once. Write function references as qualified names such as `RunConfig::createFromCommandLine` when an unqualified name has multiple definitions. These forms let publication resolve the reference deterministically while keeping project Markdown readable outside the wiki.

The [Publish documentation wiki](../../.github/workflows/publish-wiki.yml) action validates the generated Wiki for matching pull requests. After matching documentation changes reach `dev` or `main`, it builds into a temporary directory, checks out the separate `<repository>.wiki.git` repository, synchronizes the generated tree, and pushes only when content changed. Generated source links point to the branch that triggered publication, so the public Wiki can follow active development before the project is ready to merge into `main`.

The action obtains the current fork as `OWNER/NAME` from GitHub Actions' `GITHUB_REPOSITORY` value. The README uses repository-relative `../../wiki` links, so neither the documentation nor its publication configuration embeds one definitive repository URL.

## Initial GitHub setup

1. Enable **Wikis** under the public repository's **Settings → Features**.
2. Initialize the separate Wiki Git repository once. GitHub does not create `<repository>.wiki.git` until an initial page is saved through the Wiki screen, and it provides no REST or GraphQL API for this bootstrap. A repository administrator or authenticated browser automation can save the generated `Home.md` content as that first page. The publication action replaces it afterward; do not maintain it manually.
3. Push a matching documentation change to `dev` or `main`. Manual runs become available when GitHub recognizes the workflow on the default branch, but they are not required for `dev` publication.

The action first uses its repository-scoped `GITHUB_TOKEN`. If repository policy does not permit that token to push the wiki Git repository, create a repository secret named `WIKI_TOKEN` containing a narrowly scoped token with permission to write this repository; the workflow automatically prefers that secret when present.

## Local preview

From the repository root:

```bash
wiki_preview="$(mktemp -d)"
python3 dev-tools/wiki/build_wiki.py \
    --output "$wiki_preview" \
    --repository ORGANIZATION/REPOSITORY \
    --branch main
```

Replace `ORGANIZATION/REPOSITORY` with the GitHub location of the current fork. The publication workflow supplies this value automatically. The command prints the generated page count. Inspect the temporary directory as Markdown, then remove it when it is no longer needed. Generation rejects a destination that is the checkout itself or one of its ancestors, unresolved local links, unmatched or unused footnotes, missing required source pages, and colliding Wiki filenames. Review generated citations and source links after adding a reference, otherwise ambiguous filename, or function name; use an explicit repository path or qualified function name in the project prose to select one definition.
