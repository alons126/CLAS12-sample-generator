# Preparing a publication PR

For example, run **Prepare release candidate** after a change reaches `dev`. The action copies the application files from that exact `dev` commit into `release-candidate` and opens a PR into `main`. Your workstation files and the `dev` branch remain unchanged. The action never merges the PR.

The action runs only when requested through **Run workflow**. A local commit or a push to `dev` does not start it automatically.

## Files included in the PR

The action exports only committed files. It applies [the publication exclusion list](../../.github/release-excludes.txt) to that export before building it. The list excludes repository instruction files, historical source directories, historical notes, submodule declarations, and test implementations and fixtures. No submodules are downloaded. A submodule outside the historical source directory stops publication so its dependency can be reviewed explicitly.

Application code, configuration, detector resources, tutorials, and developer documentation remain included. The external geometry header remains included because both LUND applications use it. The wiki builder and its action remain included because they publish the manual. Editor settings also remain included; add a specific exclusion if they should stay only on `dev`.

The exclusion list uses rsync patterns. A leading `/` selects a path at the repository root. A trailing `/` selects directories. Other names can match at any depth. Add an explicit pattern when introducing development files with a different naming convention. The action cannot infer whether arbitrary code is a test from its contents. The electron-tester sample profiles remain included because they are application inputs.

## One-time setup

GitHub requires a manually triggered workflow to exist on the default branch before it can be dispatched. Submit a small setup PR containing [the workflow](../../.github/workflows/prepare-release.yml) and its exclusion list into `main` first. Keep identical copies on `dev`, which supplies the publication files. The setup PR does not need the application changes. See GitHub's [manual workflow instructions](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow).

Under **Settings → Actions → General → Workflow permissions**, enable **Allow GitHub Actions to create and approve pull requests**. The publication job requests repository-content and pull-request write permissions; the validation job has read permission. Repository or organization rules must permit writes to `release-candidate`. See GitHub's [Actions settings reference](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/enabling-features-for-your-repository/managing-github-actions-settings-for-a-repository).

Reserve `release-candidate` for this action. The action refuses to replace an existing branch unless its last commit has the action's `Release candidate from ` subject prefix. Do not add manual commits there. Change `dev` and run the action again instead.

## Run and review

1. Commit and push the intended application and documentation changes to `dev`.
2. Open **Actions → Prepare release candidate → Run workflow**. Select `dev` as the workflow branch. The action always reads the remote `dev` branch and records the commit it checked out.
3. Wait for validation. The action installs ROOT 6.34 and a C++ compiler through conda-forge, configures a Release build with the default workflow options, builds both LUND applications, checks their help, and installs the output into a temporary directory. It leaves CMake's default test setting enabled so missing optional files cannot silently become a required dependency. It then builds the wiki from the filtered files and checks local documentation links.
4. Review the resulting `release-candidate → main` PR. Validation covers compilation, installation, help, and documentation links. It also creates a four-event electron-tester sample with monitoring and resolves its GCARD and YAML from the publication files. It does not load ifarm software, submit jobs, run a detector simulation, or establish scientific suitability for an analysis.
5. Merge the PR when its changes are approved. Keep development material on `dev`; do not merge publication cleanup back into that branch.

The publication commit is based on `main`, rather than on the development commits. Git history on `main` therefore receives the filtered snapshot without adding the development branch's history. Files removed from a new snapshot may still exist in older history already on `main`; this action does not rewrite that history.

## Repeated runs and failures

The action creates one publication commit on the current `main` and replaces the remote `release-candidate` branch using `--force-with-lease`. This Git option permits replacement only if the remote branch still has the commit observed by the action. If another writer changes it, the push fails rather than discarding that change. Replacing the candidate removes earlier candidate commits from that branch; their source commits remain on `dev`.

An identical candidate reuses its current commit. An existing open PR receives the new description. After the PR is merged, a later publication with changed files creates a new PR using the same branch name. If the filtered files already match `main`, the action creates no commit or PR and leaves any previous candidate branch and PR unchanged.

A failed build or wiki check stops the action before any branch push. Fix the source or links on `dev` and rerun it. The action does not rewrite documentation automatically. A failed push leaves the PR unchanged. A failed PR creation can leave a pushed candidate branch; rerunning retries PR creation without needing a different branch.

The validated source archive is retained as a workflow artifact for seven days. The build, installation, and generated wiki directories exist only on the temporary runner. No samples or local workstation directories are deleted.

PR checks created by the built-in token may require a collaborator to approve workflow execution, and token-generated pushes do not start ordinary push workflows. Review GitHub's [workflow trigger rules](https://docs.github.com/en/actions/how-tos/writing-workflows/choosing-when-your-workflow-runs/triggering-a-workflow) if a downstream check does not start. The preparation action performs its own validation before pushing.
