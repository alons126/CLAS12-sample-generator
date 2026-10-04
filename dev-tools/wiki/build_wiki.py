#!/usr/bin/env python3

"""Build GitHub Wiki pages from the repository's Markdown files.

Purpose:
    Keep one editable documentation source and prepare a linked copy for GitHub Wiki publication.

Execution flow:
    Find source pages and choose unique Wiki names -> collect file and function locations -> convert
    citations and rewrite links -> write pages, sidebar, and footer -> remove unexpected top-level
    Markdown pages from the dedicated output directory.

Inputs:
    The repository README, non-ignored docs/**/*.md pages, tutorial index, sample-profile reference,
    and launcher-settings reference. Git must be available to identify ignored documentation drafts.
    The GitHub OWNER/NAME and branch are supplied by the caller; no remote is guessed from this checkout.

Outputs:
    A flat wiki directory. Existing non-Markdown files and `.git` stay in place. File links use the
    selected branch, and function links include their current source line. Each sidebar section
    follows its source index's reading order, with unlisted pages placed afterward.

Failure:
    Missing files, broken local paths, duplicate page names, invalid citations, Git failures, and write
    failures stop the build. The output cannot be the checkout root or an ancestor of it. This is not a
    general directory-safety check: use a dedicated temporary directory or Wiki checkout, never a source
    directory. Pages are written one at a time; failure can leave a partial generated tree. This script
    neither commits nor pushes it.

Markdown contract:
    Use ordinary inline Markdown links, single-backtick code spans, triple-backtick fenced blocks, and
    one-line footnote definitions. The regular expressions below recognize these repository conventions;
    they are not a complete Markdown or programming-language parser. Heading anchors and remote URLs
    are preserved, not checked for validity or availability.

CLI options:
    --output DIRECTORY       Write the generated wiki tree here (required).
    --repository OWNER/NAME  Set the public GitHub repository used by source links (required).
    --branch NAME            Set the linked repository branch (default: main).
    --help                   Print this interface without writing files.
"""

import argparse
from collections import defaultdict
import os
from pathlib import Path
import re
import subprocess
import sys
import urllib.parse

# Error presentation ----------------------------------------------------------------------------------------------------------------------------------------------------

# region Error presentation
# Decode the palette exported by run.csh. Direct invocation without that palette prints plain text.
ERROR_COLOR = os.environ.get("ERROR_COLOR", "").replace(r"\033", "\033")
RESET_COLOR = os.environ.get("RESET_COLOR", "").replace(r"\033", "\033")

def print_error(message):
    """Print one prefix-free message with the standard colored error label."""

    print(f"{ERROR_COLOR}Error:{RESET_COLOR} {message}", file=sys.stderr)

class WikiArgumentParser(argparse.ArgumentParser):
    """Use argparse's normal option handling with the repository's error presentation.

    Usage:
        parser() creates this parser once per invocation. Invalid options print usage and exit with
        status 2 before any Wiki output is written; help retains argparse's normal status 0.
    """

    def error(self, message):
        """Print usage and the diagnostic, then exit with argparse status 2."""

        self.print_usage(sys.stderr)
        print_error(message)
        self.exit(2)

# endregion

# Repository inputs -----------------------------------------------------------------------------------------------------------------------------------------------------

# region Repository inputs
# Locate inputs from the script, not the terminal's current directory.
ROOT = Path(__file__).resolve().parents[2]

# Fixed names keep the Wiki home and cross-directory reference pages stable. Other docs pages use
# their path below docs/, with directory separators replaced by hyphens.
SPECIAL_PAGES = {
    Path("README.md"): "Repository-Overview.md",
    Path("docs/index.md"): "Home.md",
    Path("tutorials/README.md"): "Workflow-Examples.md",
    Path("config/samples/README.md"): "Sample-Profiles.md",
    Path("config/run.json.md"): "Launcher-Settings.md",
}

# GitHub resolves this README link against the current repository, so it also works in a fork.
README_WIKI_PREFIX = "../../wiki"

# Section labels and directory names define sidebar groups; each section index defines its page order.
SIDEBAR_SECTIONS = (
    ("START HERE", "getting-started"),
    ("CREATE LUND FILES", "create-lund"),
    ("SUBMIT SIMULATION", "submit-simulation"),
    ("CONCEPTS", "concepts"),
    ("DEVELOPMENT", "development"),
)

# Match the Markdown forms used by the source pages. Split fenced examples before converting citations
# or adding inline-code links, so literal commands are not treated as prose references.
LINK = re.compile(r"(?P<prefix>!?\[[^\]]*\]\()(?P<target><[^>]+>|[^)\s]+)(?P<suffix>[^)]*\))")
INLINE_CODE = re.compile(r"`(?P<code>[^`\n]+)`")
INLINE_CODE_SPAN = re.compile(r"(`[^`\n]+`)")
FENCED_CODE = re.compile(r"(^```[^\n]*\n.*?^```[ \t]*$)", re.MULTILINE | re.DOTALL)
FOOTNOTE_DEFINITION = re.compile(r"^\[\^(?P<label>[^\]\n]+)\]:[ \t]*(?P<body>.+)$", re.MULTILINE)
FOOTNOTE_REFERENCE = re.compile(r"\[\^(?P<label>[^\]\n]+)\]")
PYTHON_FUNCTION = re.compile(r"^[ \t]*(?:async[ \t]+)?def[ \t]+(?P<name>[A-Za-z_][A-Za-z0-9_]*)[ \t]*\(", re.MULTILINE)
SHELL_FUNCTION = re.compile(r"^[ \t]*(?:function[ \t]+)?(?P<name>[A-Za-z_][A-Za-z0-9_]*)[ \t]*\(\)[ \t]*\{", re.MULTILINE)
CPP_FUNCTION = re.compile(
    r"^[ \t]*(?!if\b|else\b|for\b|while\b|switch\b|catch\b|explicit\b)"
    r"(?:[A-Za-z_][A-Za-z0-9_:<>,*&~]*[ \t]+)+"
    r"(?P<name>[A-Za-z_~][A-Za-z0-9_~]*(?:::[A-Za-z_~][A-Za-z0-9_~]*)*)[ \t]*"
    r"\([^;{}]*\)[ \t\n]*(?:const[ \t\n]*)?(?:noexcept[ \t\n]*)?(?:->[^{};]+)?\{",
    re.MULTILINE,
)

# These indexes aid linking only; they do not decide which Markdown pages are published. Function
# matching recognizes common definitions, not every constructor, overload, macro, or language feature.
SOURCE_SUFFIXES = {".C", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".py", ".sh", ".csh"}
LINKABLE_SHORT_SUFFIXES = SOURCE_SUFFIXES | {".conf", ".md", ".txt", ".yaml", ".yml"}
IGNORED_REFERENCE_PARTS = {".git", "build", "test-runs", "__pycache__"}
# endregion

# Page discovery --------------------------------------------------------------------------------------------------------------------------------------------------------

# region Page discovery
def git_ignored_sources(sources):
    """Return absolute candidate paths excluded by Git's ignore rules.

    Inputs:
        Absolute Markdown paths below ROOT. NUL-separated names support spaces in local draft names.

    Failure:
        A missing Git executable or an unsuccessful check raises an exception. Git status 1 means
        that no input was ignored, not that the check failed. Tracked files remain publication inputs.
    """

    relative_paths = [source.relative_to(ROOT).as_posix() for source in sources]

    if not relative_paths:
        return set()

    result = subprocess.run(
        ["git", "check-ignore", "-z", "--stdin"],
        cwd=ROOT,
        input="\0".join(relative_paths) + "\0",
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode not in (0, 1):
        diagnostic = result.stderr.strip() or "git check-ignore failed"
        raise RuntimeError(diagnostic)

    return {(ROOT / path).resolve() for path in result.stdout.split("\0") if path}

def source_pages():
    """Map each repository Markdown source to its wiki filename.

    Execution flow:
        Add the pages with fixed names, then add every remaining non-ignored documentation page. Give
        each page a unique filename because a GitHub Wiki keeps all pages in one directory.

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

    documentation_sources = sorted((ROOT / "docs").rglob("*.md"))
    ignored_sources = git_ignored_sources(documentation_sources)

    for source in documentation_sources:
        resolved = source.resolve()
        relative = source.relative_to(ROOT / "docs")

        if resolved not in pages and resolved not in ignored_sources:
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
        branch: Branch containing the repository source.
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

def convert_citations(text, source):
    """Convert GitHub footnotes into linked references supported by GitHub Wikis.

    Workflow:
        Read one-line footnote definitions outside fenced examples -> number citations by first use ->
        replace each marker with a linked superscript -> append a linked References section.

    Args:
        text: Complete source Markdown before link rewriting.
        source: Absolute Markdown source path used in validation diagnostics.

    Returns:
        Markdown with Wiki-compatible citations and one final newline. Trailing whitespace at the end
        of the page is removed even when no citations exist.

    Raises:
        ValueError: If a definition is duplicated or if a citation and definition do not match.

    Notes:
        Repository Markdown keeps native footnotes. This conversion affects only generated Wiki pages
        because GitHub does not support footnote rendering in Wikis. Fenced examples stay unchanged.
    """

    segments = FENCED_CODE.split(text)
    definitions = {}

    def remove_definition(match):
        """Store and remove one source footnote definition."""

        label = match.group("label")

        if label in definitions:
            raise ValueError(f"Duplicate footnote definition in {source.relative_to(ROOT)}: {label}")

        definitions[label] = match.group("body")
        return ""

    for index in range(0, len(segments), 2):
        segments[index] = FOOTNOTE_DEFINITION.sub(remove_definition, segments[index])

    # Labels need not be numeric. Assign numbers in reading order and keep a backlink to each label's
    # first occurrence, even when the same reference is cited again later on the page.
    order = []
    numbers = {}
    occurrences = defaultdict(int)

    def replace_reference(match):
        """Replace one source marker with its numbered Wiki link."""

        label = match.group("label")

        if label not in definitions:
            raise ValueError(f"Undefined footnote in {source.relative_to(ROOT)}: {label}")

        if label not in numbers:
            numbers[label] = len(order) + 1
            order.append(label)

        number = numbers[label]
        occurrences[label] += 1
        anchor = f'<a name="citation-{number}"></a>' if occurrences[label] == 1 else ""
        return f'{anchor}[<sup>{number}</sup>](#reference-{number})'

    for index in range(0, len(segments), 2):
        spans = INLINE_CODE_SPAN.split(segments[index])

        for span_index in range(0, len(spans), 2):
            spans[span_index] = FOOTNOTE_REFERENCE.sub(replace_reference, spans[span_index])

        segments[index] = "".join(spans)

    unused = set(definitions) - set(order)

    if unused:
        labels = ", ".join(sorted(unused))
        raise ValueError(f"Unused footnote definition in {source.relative_to(ROOT)}: {labels}")

    converted = "".join(segments).rstrip()

    if not order:
        return converted + "\n"

    references = ["## References"]

    for label in order:
        number = numbers[label]
        references.append(
            f'<a name="reference-{number}"></a>**{number}.** {definitions[label]} '
            f'[&#8617;](#citation-{number})'
        )

    return converted + "\n\n" + "\n\n".join(references) + "\n"

def repository_references():
    """Collect repository paths that documentation may link to.

    Returns:
        All usable paths and a lookup for short path names. Build output, Git data, and Python caches
        are excluded.
    """

    paths = set()
    suffixes = defaultdict(list)

    # A suffix such as core/lund/Event.h can resolve even when prose omits src/workflows/lund-creation/.
    # Keep all candidates here; the resolver, not traversal order, decides whether a name is unambiguous.
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
        Function names mapped to source paths and one-based lines. Callers link only names with one
        clear match. C++ qualified names also get a short-name entry, except constructors/destructors.

    Limits:
        Regex matches are linking hints, not a semantic API index. Python methods are indexed by their
        bare name, and C++ overloads can make a name ambiguous. Unrecognized definitions need explicit
        source links in the documentation.
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
    """Resolve an inline path without guessing among multiple shortened matches.

    Workflow:
        Reject commands and placeholders -> try the repository root and source-page directory ->
        accept a unique eligible path suffix. A trailing slash allows a bare directory name.

    Returns:
        An absolute existing indexed path, or None for an unknown, ambiguous, or excluded value.
    """

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
    """Resolve a function call or C++ qualified name to one source location.

    Workflow:
        Match the displayed name, ignoring call arguments -> prefer its exact indexed name -> try
        the short name if a qualified name was not indexed -> reject zero or multiple matches.

    Returns:
        A (path, one-based line) pair, or None. This does not verify signatures or class ownership.
    """

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

    Execution flow:
        Keep code blocks and existing links unchanged -> try a repository path -> try a unique function
        name -> write a GitHub link when one match exists.

    Inputs:
        text: Complete Markdown page after ordinary local-link rewriting.
        source: Absolute Markdown source path, used for relative file references.
        repository: Public GitHub repository written as OWNER/NAME.
        branch: Public source branch.
        paths: Eligible repository paths.
        suffixes: Shortened-path lookup, which may contain several candidates per name.
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
        pages: Absolute repository-source to wiki-filename mapping.
        repository: Public GitHub repository written as OWNER/NAME.
        branch: Public source branch.

    Returns:
        Markdown with wiki links for pages and repository links for other files. Fork-safe Wiki links
        in the repository README become internal Wiki links. External links and links within a page
        stay unchanged.

    Raises:
        FileNotFoundError: If a relative link names no existing local target or mapped Wiki page.
        ValueError: If a local link escapes the repository checkout.

    Limits:
        Existence is checked locally, not against Git tracking or the remote branch. Anchors are kept
        without checking headings. Unlike citation and inline-code conversion, this pass scans all
        text; fenced examples should not contain literal relative Markdown-link syntax.
    """

    def replace(match):
        """Rewrite one local Markdown link for the generated wiki."""

        raw_target = match.group("target")
        target = raw_target[1:-1] if raw_target.startswith("<") and raw_target.endswith(">") else raw_target

        if target.startswith("#") or urllib.parse.urlsplit(target).scheme:
            return match.group(0)

        path_text, separator, anchor = target.partition("#")
        decoded = urllib.parse.unquote(path_text)

        if source == (ROOT / "README.md").resolve() and (
            decoded == README_WIKI_PREFIX or decoded.startswith(README_WIKI_PREFIX + "/")
        ):
            requested_page = decoded.removeprefix(README_WIKI_PREFIX).lstrip("/") or "Home"
            available_pages = {Path(name).stem for name in pages.values()}

            if requested_page not in available_pages:
                raise FileNotFoundError(f"Unresolved Wiki link in README.md: {target}")

            fragment = f"#{anchor}" if separator else ""
            return match.group("prefix") + requested_page + fragment + match.group("suffix")

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
    return f"> This page is generated from [{relative.as_posix()}]({url}). Do not edit it directly; change the repository source and let automation publish the Wiki.\n\n"
# endregion

# Output publication ----------------------------------------------------------------------------------------------------------------------------------------------------

# region Output publication
def validate_output(output):
    """Resolve output and reject the checkout root and every ancestor of it.

    Limits:
        Source subdirectories and unrelated existing directories are not rejected. The caller must
        choose a dedicated generated-output directory because build() replaces and removes Markdown.
    """

    resolved = output.resolve()

    if resolved == ROOT or resolved in ROOT.parents:
        raise ValueError(f"Refusing unsafe wiki output directory: {resolved}")

    return resolved

def ordered_section_pages(directory, pages):
    """Order a sidebar section by its index links, then by remaining Wiki names.

    Inputs:
        A docs section directory name and the source-path-to-Wiki-name mapping.

    Outputs:
        Wiki names, each once, with the section overview first. Links outside the
        section do not move pages from another section into this one.

    Failure:
        An unreadable section index raises the underlying file error.
    """

    section = ROOT / "docs" / directory
    members = {source: name for source, name in pages.items() if section in source.parents}
    index = section / "index.md"
    ordered = [index] if index in members else []

    if index in members:
        for match in LINK.finditer(index.read_text(encoding="utf-8")):
            target = urllib.parse.urlsplit(match.group("target").strip("<>"))

            if target.scheme or target.netloc or not target.path:
                continue

            source = (section / urllib.parse.unquote(target.path)).resolve()

            if source in members and source not in ordered:
                ordered.append(source)

    remaining = sorted((source for source in members if source not in ordered), key=lambda source: members[source].casefold())

    return [members[source] for source in ordered + remaining]

def build(output, repository, branch):
    """Generate all pages in the flat wiki directory.

    Execution flow:
        Check the output path -> collect source references -> convert pages -> write navigation ->
        delete every unexpected top-level Markdown page.

    Inputs:
        output: Dedicated temporary directory or Wiki checkout, not a source or notes directory.
        repository: GitHub OWNER/NAME used for links; this function does not contact GitHub.
        branch: Source branch used for links, not a branch this script checks out or publishes.

    Returns:
        Number of content pages, not counting sidebar and footer.

    Side effects:
        Create output if missing, overwrite expected Markdown pages, and remove other top-level .md
        files only after all pages and navigation have been written. Preserve nested directories,
        non-Markdown files, and .git. No commit, network publication, or source editing occurs.

    Failure:
        Input and filesystem exceptions propagate. Writes are not atomic as a group: a late failure
        leaves some pages updated. Generate and validate in a temporary tree before publishing it.
    """

    output = validate_output(output)
    pages = source_pages()
    paths, suffixes = repository_references()
    definitions = function_references(paths)
    output.mkdir(parents=True, exist_ok=True)
    titles = {}

    # Citation definitions can themselves contain local links, so convert citations before rewriting
    # links. Add automatic inline-code links last, after explicit source links have their final targets.
    for source, wiki_name in pages.items():
        original = source.read_text(encoding="utf-8")
        titles[wiki_name] = page_title(original, wiki_name)
        converted = convert_citations(original, source)
        converted = rewrite_links(converted, source, pages, repository, branch)
        converted = link_code_references(converted, source, repository, branch, paths, suffixes, definitions)
        (output / wiki_name).write_text(generated_notice(repository, branch, source) + converted, encoding="utf-8")

    # Sidebar labels come from source headings. Section indexes control reading order; the fixed
    # cross-directory references are inserted in their relevant sections rather than becoming orphans.
    sidebar = ["# CLAS12 Sample Generator", ""]
    sidebar.extend((f"- [{titles['Home.md']}](Home)", f"- [{titles['Repository-Overview.md']}](Repository-Overview)"))

    for heading, directory in SIDEBAR_SECTIONS:
        members = ordered_section_pages(directory, pages)
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

    # Renaming or removing a source must also remove its old Wiki page. The output is dedicated to this
    # builder: do not assume an unexpected Markdown page is safe to keep as a hand-edited Wiki page.
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

    result = WikiArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    result.add_argument("--output", required=True, type=Path)
    result.add_argument("--repository", required=True)
    result.add_argument("--branch", default="main")

    return result

def main():
    """Validate link settings, generate pages, and print the content-page count.

    Failure:
        argparse owns invalid-option status 2. Invalid repository/branch values and build errors
        propagate to the final boundary below, which prints one error and exits with status 1.
        Interruption exits with status 130. No failure creates a commit or publishes the output.
    """

    args = parser().parse_args()

    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repository):
        raise ValueError("--repository must use the OWNER/NAME form")

    if not args.branch or any(character.isspace() for character in args.branch):
        raise ValueError("--branch must be nonempty and contain no whitespace")

    count = build(args.output, args.repository, args.branch)
    print(f"Generated {count} wiki pages in {args.output.resolve()}")

if __name__ == "__main__":
    # Keep exceptions free of presentation prefixes; only this final boundary labels them as errors.
    try:
        main()
    except KeyboardInterrupt:
        print_error("Interrupted.")
        sys.exit(130)
    except Exception as error:
        print_error(error)
        sys.exit(1)
# endregion
