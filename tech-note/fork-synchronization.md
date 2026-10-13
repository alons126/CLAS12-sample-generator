# Synchronizing tags and releases with the collaboration fork

For example, publishing `v1.0.1` in the official repository does not automatically add its tag or release page to the collaboration fork. First copy the tag, then copy the release title, notes, and any uploaded files. The fork will show the same version pointing to the same commit.

The official source is [alons126/CLAS12-sample-tools](https://github.com/alons126/CLAS12-sample-tools). The destination is [e4nu/CLAS12-sample-tools](https://github.com/e4nu/CLAS12-sample-tools). Issue official versions in the source repository first. Fork release pages are copies of those versions. Copying a release assigns a new GitHub release ID and publication time in the fork; Git tag objects and their commit identities stay the same.

## Requirements

Use a local clone with `origin` pointing to the official repository. You need Git, [GitHub CLI](https://cli.github.com/), `jq` for reading JSON data, and write access to the fork. Run the commands below in Bash. Sign in to GitHub CLI with `gh auth login` before using the release-copy commands. Git authentication and GitHub CLI authentication are separate; a successful Git push does not establish a CLI login.

Check the source remote:

```bash
git remote get-url origin
```

It should identify `https://github.com/alons126/CLAS12-sample-tools.git`. If it points elsewhere, choose the official source remote before proceeding.

## Copy all tags

A tag is a named Git reference to a commit or an annotated tag object. The following commands fetch every official tag and push every local tag into the fork:

```bash
git \
    fetch \
    origin \
    'refs/tags/*:refs/tags/*'

git \
    push \
    --atomic \
    https://github.com/e4nu/CLAS12-sample-tools.git \
    'refs/tags/*:refs/tags/*'
```

This includes `v1.0.0`, `v1.0.1`, and `legacy-code-archive`. It copies annotated tag objects and signatures without creating commits or updating branches. Because the push selects all local tags, it also sends any local-only tags: inspect `git tag --list` first and use a clone containing only intended source tags. Fetching does not remove stale local tags.

Neither command forces tag replacement. A conflicting local tag stops the fetch. A conflicting fork tag stops the atomic push, which updates all selected tags or none. Investigate the conflict rather than adding `--force`. These commands do not delete tags from either repository.

To compare tag object IDs, run these commands and compare the IDs for each shared name:

```bash
git \
    ls-remote \
    --tags \
    --refs \
    https://github.com/alons126/CLAS12-sample-tools.git

git \
    ls-remote \
    --tags \
    --refs \
    https://github.com/e4nu/CLAS12-sample-tools.git
```

## Copy all published releases

GitHub release pages are repository records, so `git push` does not copy them. The script below reads all source release pages through GitHub CLI, skips drafts, and creates missing copies on the already-synchronized tags. It preserves titles, notes, prerelease status, and uploaded attachment contents. GitHub generates the source ZIP and tar.gz downloads from each tag; do not upload those automatic archives as extra attachments.

Run this only after the tag-copy step succeeds. The script exits on the first failed command. It skips any release page already present in the fork, so it does not overwrite independently edited pages or update existing attachments. Review existing pages separately if the official release notes changed after the first copy.

```bash
set -euo pipefail
source_repository=alons126/CLAS12-sample-tools
fork_repository=e4nu/CLAS12-sample-tools
release_copy_dir="$(mktemp -d)"

gh api \
    --paginate \
    --slurp \
    "repos/$source_repository/releases?per_page=100" > "$release_copy_dir/source-pages.json"

gh api \
    --paginate \
    --slurp \
    "repos/$fork_repository/releases?per_page=100" > "$release_copy_dir/fork-pages.json"

jq '[.[][] | select(.draft == false)] | reverse' "$release_copy_dir/source-pages.json" > "$release_copy_dir/releases.json"
jq '[.[][] | .tag_name]' "$release_copy_dir/fork-pages.json" > "$release_copy_dir/existing-tags.json"

latest_tag="$(gh api \
    "repos/$source_repository/releases/latest" \
    --jq '.tag_name')"

while IFS= read -r release; do
    tag="$(jq -r '.tag_name' <<< "$release")"

    if jq -e --arg tag "$tag" 'index($tag) != null' "$release_copy_dir/existing-tags.json" > /dev/null; then
        printf 'Already present: %s\n' "$tag"
        continue
    fi

    title="$(jq -r '.name // .tag_name' <<< "$release")"
    jq -r '.body // ""' <<< "$release" > "$release_copy_dir/notes.md"
    asset_dir="$(mktemp -d "$release_copy_dir/assets.XXXXXX")"
    release_flags=(--latest=false)
    assets=()

    if [[ "$tag" == "$latest_tag" ]]; then
        release_flags=(--latest)
    fi

    if [[ "$(jq -r '.prerelease' <<< "$release")" == true ]]; then
        release_flags+=(--prerelease)
    fi

    if [[ "$(jq '.assets | length' <<< "$release")" -gt 0 ]]; then
        gh release download "$tag" \
            --repo "$source_repository" \
            --dir "$asset_dir"
        shopt -s nullglob dotglob
        assets=("$asset_dir"/*)
        shopt -u nullglob dotglob
    fi

    gh release create "$tag" \
        --repo "$fork_repository" \
        --verify-tag \
        --title "$title" \
        --notes-file "$release_copy_dir/notes.md" \
        "${release_flags[@]}" \
        "${assets[@]}"
done < <(jq -c '.[]' "$release_copy_dir/releases.json")

printf 'Temporary notes and downloaded attachments: %s\n' "$release_copy_dir"
```

The script assumes the source has at least one published non-prerelease release for GitHub's `releases/latest` lookup. It does not copy draft pages, original release authors, timestamps, discussions, immutable-release settings, or custom attachment labels. Downloaded attachment filenames and bytes are preserved. For special asset names containing `#`, GitHub CLI treats that character as an attachment-label separator; handle those attachments separately with the GitHub API. See the CLI references for [release creation](https://cli.github.com/manual/gh_release_create) and [attachment downloads](https://cli.github.com/manual/gh_release_download).

Each release is copied separately. If a later copy fails, earlier successful copies remain. A failed attachment upload can leave a partial release page; inspect it before rerunning because existing pages are skipped. The temporary directory contains only notes, API metadata, and downloaded attachments. Remove that exact directory after verification; the commands do not delete workstation source files.

## Verify and repeat

Open the [fork's releases page](https://github.com/e4nu/CLAS12-sample-tools/releases). Check the titles, notes, prerelease flags, uploaded files, and latest-release label against the [official releases page](https://github.com/alons126/CLAS12-sample-tools/releases). If an existing fork release should become latest, adjust it explicitly with `gh release edit TAG --repo e4nu/CLAS12-sample-tools --latest` after verifying the official latest version.

Repeat tag copying and release copying after each official publication. Code branch updates are separate: use the fork's **Sync fork** control to update its `main`, and send contributions back through PRs. These instructions synchronize tags and release pages without merging development code or changing the fork's branches.
