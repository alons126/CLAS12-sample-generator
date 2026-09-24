#!/usr/bin/env python3

#
# Created by Alon Sportes on 24/09/2026.
#

"""Build GitHub Wiki pages from the repository's maintained Markdown.

Purpose:
    Keep repository documentation as the single editable source while publishing a navigable copy in
    the public GitHub Wiki.

Workflow:
    Discover maintained pages -> assign collision-free wiki names -> rewrite local links -> write the
    generated pages -> create the sidebar and footer -> remove stale generated Markdown pages.

Inputs:
    The repository README, docs/*.md, tutorial index, sample-profile reference, and build/launcher
    references. The repository slug and default branch are explicit so source links are stable.

Outputs:
    A flat GitHub Wiki tree rooted at the requested output directory. Existing non-Markdown files and
    a possible .git directory are preserved; stale top-level Markdown pages are removed.

Failure:
    Missing source files, unresolved local links, duplicate wiki names, or an unsafe output directory
    stop generation before the page set is reported as complete.

CLI options:
    --output DIRECTORY       Write the generated wiki tree here (required).
    --repository OWNER/NAME  Set the public GitHub repository used by source links (required).
    --branch NAME            Set the linked repository branch (default: main).
    --help                   Print this interface without writing files.
"""

import argparse
from pathlib import Path
import re
import urllib.parse


# Repository inputs -----------------------------------------------------------------------------------------------------------------------------------------------------

# region Repository inputs
ROOT = Path(__file__).resolve().parents[1]

SPECIAL_PAGES = {
    Path("README.md"): "Repository-Overview.md",
    Path("docs/index.md"): "Home.md",
    Path("tutorials/README.md"): "Workflow-Examples.md",
    Path("config/samples/README.md"): "Sample-Profiles.md",
    Path("config/run.json.md"): "Launcher-Settings.md",
    Path("CMakePresets.json.md"): "CMake-Presets.md",
}

PINNED_SIDEBAR = (
    "Home.md",
    "Repository-Overview.md",
    "uniform-samples.md",
    "genie-to-lund-conversion.md",
    "gemc-reconstruction-batch-submission.md",
    "Workflow-Examples.md",
    "configuration.md",
    "building.md",
    "architecture.md",
)

LINK = re.compile(r"(?P<prefix>!?\[[^\]]*\]\()(?P<target><[^>]+>|[^)\s]+)(?P<suffix>[^)]*\))")
# endregion


# Page discovery --------------------------------------------------------------------------------------------------------------------------------------------------------

# region Page discovery
def source_pages():
    """Return the maintained Markdown-to-wiki filename mapping.

    Workflow:
        Add the explicitly renamed overview/reference pages, then add every docs/*.md file except the
        newcomer index already mapped to Home.md.

    Returns:
        Dictionary keyed by absolute source paths with flat wiki filenames as values.

    Raises:
        FileNotFoundError: If a required explicitly mapped source is missing.
        ValueError: If two sources resolve to the same case-insensitive wiki filename.
    """

    pages = {}

    for relative, wiki_name in SPECIAL_PAGES.items():
        source = ROOT / relative

        if not source.is_file():
            raise FileNotFoundError(f"Missing wiki source: {relative}")

        pages[source.resolve()] = wiki_name

    for source in sorted((ROOT / "docs").glob("*.md")):
        resolved = source.resolve()

        if resolved not in pages:
            pages[resolved] = source.name

    lowered = [name.lower() for name in pages.values()]

    if len(lowered) != len(set(lowered)):
        raise ValueError("Wiki page names collide after case normalization")

    return pages
# endregion


# Markdown conversion ---------------------------------------------------------------------------------------------------------------------------------------------------

# region Markdown conversion
def page_title(text, fallback):
    """Return the first level-one Markdown heading or a filename-derived fallback."""

    match = re.search(r"^#\s+(.+?)\s*$", text, re.MULTILINE)

    return match.group(1) if match else fallback.removesuffix(".md").replace("-", " ")


def repository_url(repository, branch, relative, fragment="", image=False):
    """Build one public URL for a repository file or directory.

    Args:
        repository: GitHub OWNER/NAME slug.
        branch: Branch containing the maintained source.
        relative: Repository-relative path to link.
        fragment: Optional existing Markdown anchor, including its leading hash.
        image: Use raw.githubusercontent.com for an embedded image when true.

    Returns:
        URL-safe public source or raw-content URL.
    """

    path = urllib.parse.quote(relative.as_posix(), safe="/-._~")
    quoted_branch = urllib.parse.quote(branch, safe="-._~/")

    if image:
        return f"https://raw.githubusercontent.com/{repository}/{quoted_branch}/{path}"

    kind = "tree" if (ROOT / relative).is_dir() else "blob"
    return f"https://github.com/{repository}/{kind}/{quoted_branch}/{path}{fragment}"


def rewrite_links(text, source, pages, repository, branch):
    """Rewrite local Markdown targets for the flat GitHub Wiki namespace.

    Inputs:
        text: Complete Markdown page text.
        source: Absolute source file owning relative links.
        pages: Absolute maintained-source to wiki-filename mapping.
        repository: Public GitHub OWNER/NAME slug.
        branch: Public source branch.

    Returns:
        Markdown with wiki-to-wiki links for generated pages and public repository URLs for other
        checked-in files. External URLs and page-local anchors are unchanged.

    Raises:
        FileNotFoundError: If a relative link names no checked-in source target.
        ValueError: If a local link escapes the repository checkout.
    """

    def replace(match):
        raw_target = match.group("target")
        target = raw_target[1:-1] if raw_target.startswith("<") and raw_target.endswith(">") else raw_target

        if target.startswith("#") or urllib.parse.urlsplit(target).scheme:
            return match.group(0)

        path_text, separator, anchor = target.partition("#")
        decoded = urllib.parse.unquote(path_text)
        resolved = (source.parent / decoded).resolve()

        try:
            relative = resolved.relative_to(ROOT)
        except ValueError as error:
            raise ValueError(f"Local link escapes repository in {source.relative_to(ROOT)}: {target}") from error

        if not resolved.exists():
            raise FileNotFoundError(f"Unresolved link in {source.relative_to(ROOT)}: {target}")

        fragment = f"#{anchor}" if separator else ""

        if resolved in pages:
            replacement = Path(pages[resolved]).stem + fragment
        else:
            replacement = repository_url(repository, branch, relative, fragment, match.group("prefix").startswith("!"))

        return match.group("prefix") + replacement + match.group("suffix")

    return LINK.sub(replace, text)


def generated_notice(repository, branch, source):
    """Return the source-of-truth notice prepended to each generated page."""

    relative = source.relative_to(ROOT)
    url = repository_url(repository, branch, relative)
    return f"> This page is generated from [{relative.as_posix()}]({url}). Edit the repository source; automated publishing replaces direct wiki edits.\n\n"
# endregion


# Output publication ----------------------------------------------------------------------------------------------------------------------------------------------------

# region Output publication
def validate_output(output):
    """Reject output paths that could overwrite the checkout or one of its ancestors."""

    resolved = output.resolve()

    if resolved == ROOT or resolved in ROOT.parents:
        raise ValueError(f"Refusing unsafe wiki output directory: {resolved}")

    return resolved


def build(output, repository, branch):
    """Generate the complete flat wiki page set.

    Workflow:
        Validate destination -> convert maintained pages -> write navigation -> delete obsolete
        top-level Markdown pages not present in the generated set.

    Returns:
        Number of generated content pages, excluding sidebar and footer.
    """

    output = validate_output(output)
    pages = source_pages()
    output.mkdir(parents=True, exist_ok=True)
    titles = {}

    for source, wiki_name in pages.items():
        original = source.read_text(encoding="utf-8")
        titles[wiki_name] = page_title(original, wiki_name)
        converted = rewrite_links(original, source, pages, repository, branch)
        (output / wiki_name).write_text(generated_notice(repository, branch, source) + converted, encoding="utf-8")

    ordered = [name for name in PINNED_SIDEBAR if name in titles]
    ordered.extend(sorted((name for name in titles if name not in ordered), key=lambda name: titles[name].casefold()))
    sidebar = ["# CLAS12 Sample Generator", ""]
    sidebar.extend(f"- [{titles[name]}]({Path(name).stem})" for name in ordered)
    sidebar.append("")
    (output / "_Sidebar.md").write_text("\n".join(sidebar), encoding="utf-8")

    repository_home = f"https://github.com/{repository}"
    footer = f"Generated from the [{repository} documentation]({repository_home}/tree/{branch}/docs)."
    (output / "_Footer.md").write_text(footer + "\n", encoding="utf-8")

    expected = set(titles) | {"_Sidebar.md", "_Footer.md"}

    for stale in output.glob("*.md"):
        if stale.name not in expected:
            stale.unlink()

    return len(pages)
# endregion


# Command-line entry point ----------------------------------------------------------------------------------------------------------------------------------------------

# region Command-line entry point
def parser():
    """Create the documented command-line parser."""

    result = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    result.add_argument("--output", required=True, type=Path)
    result.add_argument("--repository", required=True)
    result.add_argument("--branch", default="main")

    return result


def main():
    """Parse arguments, build the wiki, and report its deterministic page count."""

    args = parser().parse_args()

    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repository):
        raise ValueError("--repository must use the OWNER/NAME form")

    if not args.branch or any(character.isspace() for character in args.branch):
        raise ValueError("--branch must be nonempty and contain no whitespace")

    count = build(args.output, args.repository, args.branch)
    print(f"Generated {count} wiki pages in {args.output.resolve()}")


if __name__ == "__main__":
    main()
# endregion
