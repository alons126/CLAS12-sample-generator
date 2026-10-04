# Publishing the GitHub Wiki

The GitHub Wiki is the long-form reader manual. Repository Markdown is its version-controlled source; it is not a competing documentation set. `README.md` introduces the project, and `docs/index.md` becomes the Wiki home page.

Never edit a generated Wiki page directly. Change the repository source and let publication replace the generated Wiki.

## What is published

`dev-tools/wiki/build_wiki.py` collects:

- `README.md`;
- every non-ignored `docs/**/*.md` page;
- `tutorials/README.md`;
- `config/samples/README.md`; and
- `config/run.json.md`.

It maps the nested sources into GitHub Wiki's flat page namespace, builds the sidebar and footer, converts local documentation links to Wiki links, converts code/configuration links to repository URLs, and converts Markdown footnotes to linked numbered references.

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

The `Publish documentation wiki` action validates matching pull requests. After matching files reach `dev` or `main`, it generates the Wiki with the triggering repository and branch, checks out the separate `<repository>.wiki.git` repository, synchronizes generated content, and pushes only when it changed. No definitive fork URL is stored in the documentation.

There is one Wiki per repository, not one per branch. Publications from `dev` and `main` replace the same pages; the last successful publication determines which branch's documentation and source links readers see. A code-only push does not trigger this workflow. If source changes move automatically linked function definitions without changing documentation, run the action manually to refresh their line links.

GitHub creates the separate Wiki Git repository only after the repository Wiki has been enabled and one initial page has been saved. This is a one-time repository setup step:

1. Enable **Wikis** under **Settings → Features**.
2. Save one initial Wiki page so `<repository>.wiki.git` exists.
3. Push a matching documentation change to `dev` or `main`; the workflow replaces that initial page with generated content.

After bootstrap, collaborators edit only repository source files. If repository policy prevents the normal `GITHUB_TOKEN` from pushing the Wiki repository, add a narrowly scoped repository secret named `WIKI_TOKEN`; the workflow prefers it automatically.

## Link conventions

Use repository-relative Markdown links for documents and explicit repository paths for source. Inline-code file paths and unambiguous qualified function names can be linked automatically. Keep command examples in fenced blocks so they remain copyable. When a short filename is ambiguous, write the full repository-relative path.
