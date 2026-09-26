#!/usr/bin/env python3

#
# Created by Alon Sportes on 24/09/2026.
#

"""Build GitHub Wiki pages from the project's Markdown files.

Purpose:
    Keep one editable documentation source and publish a linked copy to the GitHub Wiki.

Workflow:
    Find source pages -> collect file and function locations -> choose unique wiki names -> rewrite
    links -> write pages, sidebar, and footer -> remove old generated Markdown pages.

Inputs:
    The repository README, docs/**/*.md, tutorial index, sample-profile reference, and build/launcher
    references. The GitHub OWNER/NAME and branch are given directly so source links stay stable.

Outputs:
    A flat wiki directory. Existing non-Markdown files and `.git` stay in place. File links use the
    selected branch, and function links include their current source line.

Failure:
    Missing files, broken local links, duplicate page names, or an unsafe output path stop the build.

CLI options:
    --output DIRECTORY       Write the generated wiki tree here (required).
    --repository OWNER/NAME  Set the public GitHub repository used by source links (required).
    --branch NAME            Set the linked repository branch (default: main).
    --help                   Print this interface without writing files.
"""

import argparse
from collections import defaultdict
from pathlib import Path
import re
import urllib.parse


# Repository inputs -----------------------------------------------------------------------------------------------------------------------------------------------------

# region Repository inputs
ROOT = Path(__file__).resolve().parents[2]

SPECIAL_PAGES = {
    Path("README.md"): "Repository-Overview.md",
    Path("docs/index.md"): "Home.md",
    Path("tutorials/README.md"): "Workflow-Examples.md",
    Path("config/samples/README.md"): "Sample-Profiles.md",
    Path("config/run.json.md"): "Launcher-Settings.md",
}

SIDEBAR_SECTIONS = (
    ("START HERE", "getting-started"),
    ("CREATE LUND FILES", "create-lund"),
    ("SUBMIT SIMULATION", "submit-simulation"),
    ("CONCEPTS", "concepts"),
    ("DEVELOPMENT", "development"),
    ("HISTORY", "history"),
)

LINK = re.compile(r"(?P<prefix>!?\[[^\]]*\]\()(?P<target><[^>]+>|[^)\s]+)(?P<suffix>[^)]*\))")
INLINE_CODE = re.compile(r"`(?P<code>[^`\n]+)`")
FENCED_CODE = re.compile(r"(^```[^\n]*\n.*?^```[ \t]*$)", re.MULTILINE | re.DOTALL)
PYTHON_FUNCTION = re.compile(r"^[ \t]*(?:async[ \t]+)?def[ \t]+(?P<name>[A-Za-z_][A-Za-z0-9_]*)[ \t]*\(", re.MULTILINE)
SHELL_FUNCTION = re.compile(r"^[ \t]*(?:function[ \t]+)?(?P<name>[A-Za-z_][A-Za-z0-9_]*)[ \t]*\(\)[ \t]*\{", re.MULTILINE)
CPP_FUNCTION = re.compile(
    r"^[ \t]*(?!if\b|else\b|for\b|while\b|switch\b|catch\b|explicit\b)"
    r"(?:[A-Za-z_][A-Za-z0-9_:<>,*&~]*[ \t]+)+"
    r"(?P<name>[A-Za-z_~][A-Za-z0-9_~]*(?:::[A-Za-z_~][A-Za-z0-9_~]*)*)[ \t]*"
    r"\([^;{}]*\)[ \t\n]*(?:const[ \t\n]*)?(?:noexcept[ \t\n]*)?(?:->[^{};]+)?\{",
    re.MULTILINE,
)

SOURCE_SUFFIXES = {".C", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".py", ".sh", ".csh"}
LINKABLE_SHORT_SUFFIXES = SOURCE_SUFFIXES | {".conf", ".md", ".txt", ".yaml", ".yml"}
IGNORED_REFERENCE_PARTS = {".git", "build", "test-runs", "__pycache__"}
# endregion


# Page discovery --------------------------------------------------------------------------------------------------------------------------------------------------------

# region Page discovery
def source_pages():
    """Map each maintained Markdown source to its wiki filename.

    Workflow:
        Add the pages with fixed names, then add every remaining documentation page. Give each page a
        unique filename because a GitHub Wiki keeps all pages in one directory.

    Returns:
        Absolute source paths mapped to flat wiki filenames.

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

    for source in sorted((ROOT / "docs").rglob("*.md")):
        resolved = source.resolve()

        if resolved not in pages:
            relative = source.relative_to(ROOT / "docs")
            parts = list(relative.with_suffix("").parts)

            if parts[-1] == "index":
                parts[-1] = "overview"

            pages[resolved] = "-".join(parts) + ".md"

    lowered = [name.lower() for name in pages.values()]

    if len(lowered) != len(set(lowered)):
        raise ValueError("Wiki page names collide after case normalization")

    return pages
# endregion


# Markdown conversion ---------------------------------------------------------------------------------------------------------------------------------------------------

# region Markdown conversion
def page_title(text, fallback):
    """Return the first page title, or build one from the filename."""

    match = re.search(r"^#\s+(.+?)\s*$", text, re.MULTILINE)

    return match.group(1) if match else fallback.removesuffix(".md").replace("-", " ")


def repository_url(repository, branch, relative, fragment="", image=False):
    """Build a public URL for one repository path.

    Args:
        repository: GitHub repository written as OWNER/NAME.
        branch: Branch containing the maintained source.
        relative: Repository-relative path to link.
        fragment: Optional existing Markdown anchor, including its leading hash.
        image: Use raw.githubusercontent.com for an embedded image when true.

    Returns:
        Public source URL, or raw-content URL for an image.
    """

    path = urllib.parse.quote(relative.as_posix(), safe="/-._~")
    quoted_branch = urllib.parse.quote(branch, safe="-._~/")

    if image:
        return f"https://raw.githubusercontent.com/{repository}/{quoted_branch}/{path}"

    kind = "tree" if (ROOT / relative).is_dir() else "blob"
    return f"https://github.com/{repository}/{kind}/{quoted_branch}/{path}{fragment}"


def repository_references():
    """Collect repository paths that documentation may link to.

    Returns:
        All usable paths and a lookup for short path names. Build output, Git data, and Python caches
        are excluded.
    """

    paths = set()
    suffixes = defaultdict(list)

    for path in ROOT.rglob("*"):
        if any(part in IGNORED_REFERENCE_PARTS for part in path.relative_to(ROOT).parts):
            continue

        resolved = path.resolve()
        paths.add(resolved)
        relative = path.relative_to(ROOT).as_posix()
        parts = relative.split("/")

        for index in range(len(parts)):
            suffixes["/".join(parts[index:])].append(resolved)

    return paths, suffixes


def function_references(paths):
    """Collect function definitions by full and short name.

    Inputs:
        paths: Eligible repository files and directories returned by `repository_references`.

    Returns:
        Function names mapped to source paths and lines. Callers link only names with one clear match.
    """

    definitions = defaultdict(list)

    for path in sorted(paths):
        if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
            continue

        text = path.read_text(encoding="utf-8", errors="replace")

        if path.suffix == ".py":
            patterns = (PYTHON_FUNCTION,)
        elif path.suffix in {".sh", ".csh"}:
            patterns = (SHELL_FUNCTION,)
        else:
            patterns = (CPP_FUNCTION,)

        for pattern in patterns:
            for match in pattern.finditer(text):
                name = match.group("name")
                line = text.count("\n", 0, match.start("name")) + 1
                definition = (path, line)
                definitions[name].append(definition)

                if "::" in name:
                    owner, short_name = name.rsplit("::", 1)

                    if short_name != owner.rsplit("::", 1)[-1] and short_name != f"~{owner.rsplit('::', 1)[-1]}":
                        definitions[short_name].append(definition)

    return definitions


def resolve_repository_path(code, source, paths, suffixes):
    """Find the one repository path named by an inline-code value."""

    if any(character.isspace() for character in code) or any(character in code for character in "*{}<>|=$'\""):
        return None

    displayed_as_directory = code.endswith("/")
    normalized = code.removeprefix("./").rstrip("/")

    if not normalized or Path(normalized).is_absolute() or "://" in normalized:
        return None

    if "/" not in normalized and not displayed_as_directory and not Path(normalized).suffix:
        return None

    direct_candidates = ((ROOT / normalized).resolve(), (source.parent / normalized).resolve())

    for candidate in direct_candidates:
        if candidate in paths:
            return candidate

    matches = []

    for candidate in dict.fromkeys(suffixes.get(normalized, ())):
        relative = candidate.relative_to(ROOT)
        omitted_parts = len(relative.parts) - len(Path(normalized).parts)

        if "/" not in normalized or omitted_parts <= 1 or (candidate.is_file() and candidate.suffix in LINKABLE_SHORT_SUFFIXES):
            matches.append(candidate)

    return matches[0] if len(matches) == 1 else None


def resolve_function(code, definitions):
    """Find the one function definition named by an inline-code value."""

    match = re.fullmatch(r"(?P<name>[A-Za-z_~][A-Za-z0-9_~]*(?:::[A-Za-z_~][A-Za-z0-9_~]*)*)(?:\(.*\))?", code)

    if not match or ("(" not in code and "::" not in code):
        return None

    name = match.group("name")
    matches = list(dict.fromkeys(definitions.get(name, ())))

    if not matches and "::" in name:
        matches = list(dict.fromkeys(definitions.get(name.rsplit("::", 1)[-1], ())))

    return matches[0] if len(matches) == 1 else None


def link_code_references(text, source, repository, branch, paths, suffixes, definitions):
    """Link file and function names to their repository locations.

    Workflow:
        Keep code blocks and existing links unchanged -> try a repository path -> try a unique function
        name -> write a GitHub link when one match exists.

    Inputs:
        text: Complete Markdown page after ordinary local-link rewriting.
        source: Absolute Markdown source path, used for relative file references.
        repository: Public GitHub repository written as OWNER/NAME.
        branch: Public source branch.
        paths: Eligible repository paths.
        suffixes: Unique shortened-path lookup.
        definitions: Function-definition lookup with current line numbers.

    Returns:
        Markdown with links for known file and function names.

    Notes:
        Code blocks stay copyable. Unknown or unclear names stay unlinked.
    """

    def replace(match):
        """Link one inline-code match when it names a known path or function."""

        start, end = match.span()

        if start > 0 and end < len(match.string) and match.string[start - 1] == "[" and match.string[end] == "]":
            return match.group(0)

        code = match.group("code")
        resolved_path = resolve_repository_path(code, source, paths, suffixes)

        if resolved_path is not None:
            relative = resolved_path.relative_to(ROOT)
            url = repository_url(repository, branch, relative)
            return f"[`{code}`]({url})"

        definition = resolve_function(code, definitions)

        if definition is not None:
            path, line = definition
            relative = path.relative_to(ROOT)
            url = repository_url(repository, branch, relative, f"#L{line}")
            return f"[`{code}`]({url})"

        return match.group(0)

    segments = FENCED_CODE.split(text)

    for index in range(0, len(segments), 2):
        segments[index] = INLINE_CODE.sub(replace, segments[index])

    return "".join(segments)


def rewrite_links(text, source, pages, repository, branch):
    """Rewrite local Markdown links for the flat wiki directory.

    Inputs:
        text: Complete Markdown page text.
        source: Absolute source file owning relative links.
        pages: Absolute maintained-source to wiki-filename mapping.
        repository: Public GitHub repository written as OWNER/NAME.
        branch: Public source branch.

    Returns:
        Markdown with wiki links for pages and repository links for other files. External links and
        links within a page stay unchanged.

    Raises:
        FileNotFoundError: If a relative link names no checked-in source target.
        ValueError: If a local link escapes the repository checkout.
    """

    def replace(match):
        """Rewrite one local Markdown link for the generated wiki."""

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
    """Return the source notice placed at the top of each generated page."""

    relative = source.relative_to(ROOT)
    url = repository_url(repository, branch, relative)
    return f"> This page is generated from [{relative.as_posix()}]({url}). Edit the repository source; automated publishing replaces direct wiki edits.\n\n"
# endregion


# Output publication ----------------------------------------------------------------------------------------------------------------------------------------------------

# region Output publication
def validate_output(output):
    """Reject output paths that could overwrite the checkout or a parent directory."""

    resolved = output.resolve()

    if resolved == ROOT or resolved in ROOT.parents:
        raise ValueError(f"Refusing unsafe wiki output directory: {resolved}")

    return resolved


def build(output, repository, branch):
    """Generate all pages in the flat wiki directory.

    Workflow:
        Check the output path -> collect source references -> convert pages -> write navigation ->
        delete old generated Markdown pages.

    Returns:
        Number of content pages, not counting sidebar and footer.
    """

    output = validate_output(output)
    pages = source_pages()
    paths, suffixes = repository_references()
    definitions = function_references(paths)
    output.mkdir(parents=True, exist_ok=True)
    titles = {}

    for source, wiki_name in pages.items():
        original = source.read_text(encoding="utf-8")
        titles[wiki_name] = page_title(original, wiki_name)
        converted = rewrite_links(original, source, pages, repository, branch)
        converted = link_code_references(converted, source, repository, branch, paths, suffixes, definitions)
        (output / wiki_name).write_text(generated_notice(repository, branch, source) + converted, encoding="utf-8")

    sidebar = ["# CLAS12 Sample Generator", ""]
    sidebar.extend((f"- [{titles['Home.md']}](Home)", f"- [{titles['Repository-Overview.md']}](Repository-Overview)"))

    for heading, directory in SIDEBAR_SECTIONS:
        members = []

        for source, wiki_name in pages.items():
            relative = source.relative_to(ROOT)

            if len(relative.parts) >= 3 and relative.parts[:2] == ("docs", directory):
                members.append(wiki_name)

        members.sort(key=lambda name: (not name.endswith("-overview.md"), titles[name].casefold()))
        sidebar.extend(("", f"## {heading}", ""))
        sidebar.extend(f"- [{titles[name]}]({Path(name).stem})" for name in members)

        if directory == "create-lund":
            sidebar.append(f"- [{titles['Sample-Profiles.md']}](Sample-Profiles)")

        if directory == "development":
            sidebar.append(f"- [{titles['Launcher-Settings.md']}](Launcher-Settings)")

    sidebar.extend(("", "## EXAMPLES", "", f"- [{titles['Workflow-Examples.md']}](Workflow-Examples)"))
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
    """Create the command-line parser shown in this file's help."""

    result = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    result.add_argument("--output", required=True, type=Path)
    result.add_argument("--repository", required=True)
    result.add_argument("--branch", default="main")

    return result


def main():
    """Read arguments, build the wiki, and print its page count."""

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
