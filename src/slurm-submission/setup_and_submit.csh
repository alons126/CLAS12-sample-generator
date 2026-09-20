#
# Created by Alon Sportes on 19/09/2026.
#

#!/bin/tcsh

# Setup and submit ------------------------------------------------------------
# Description: validates resolved sample settings and submits one GEMC/reconstruction Slurm array for each selected sample.
# Purpose: provide the single ifarm setup/submission path for completed uniform or physical LUND samples.
# Usage: source run.csh --workflow submit --lund-dir RUN/lundfiles [--config FILE] [overrides].
# Workflow:
#   1. Ask resolve_inputs.py to validate configuration and emit private tcsh assignments.
#   2. Load the requested GEMC environment and report the resolved detector and sample inputs.
#   3. Validate every LUND file and worker command before replacing simulation output directories.
#   4. Submit the protected GEMC/reconstruction payload as a Slurm array and return its status.
# Inputs: manifest/config/CLI settings (GEMC fallback 5.14), existing OUTPATH/lundfiles/PREFIX_INDEX.txt, GCARD, YAML, and the login shell's module command.
# Outputs: Slurm jobs writing OUTPATH/mchipo and OUTPATH/reconhipo; uniform samples also recreate rootfiles.
# Failure behavior: checked failures jump to the shared return block; sourcing never exits the user's login shell.
# Printing: run.csh sources set_environment.csh first; that file owns the COLOR_* environment-variable palette used below.
# WARNING: each submission replaces the selected sample's simulation output directories. LUND is preserved.
# All paths must be absolute and contain only letters, numbers, /, _, -, and . (protected payload contract).
# JOB_NEVENTS is an event limit shared by the array, not a claim about each input file's length.

# Input resolution ------------------------------------------------------------

# region Input resolution
# Python only resolves and validates data. Module loading and sbatch remain in this sourced login shell.
# The resolver writes one whitelisted .csh assignment file per selected sample into a private temporary directory.
# --help bypasses temporary-file creation, while all other paths converge on submission_finish for cleanup.
set CLAS12_SAMPLE_STATUS = 1
set submission_environment = ""

if ($#argv == 1) then
    if ("$argv[1]" == "--help") then
        python3 src/slurm-submission/resolve_inputs.py --help
        set CLAS12_SAMPLE_STATUS = $status
        goto submission_finish
    endif
endif

set submission_environment = `mktemp -d /tmp/clas12-submit.XXXXXXXX`
if ($status != 0 || "$submission_environment" == "") goto submission_finish

python3 src/slurm-submission/resolve_inputs.py --environment-dir "$submission_environment" $argv:q
if ($status != 0) goto submission_finish

set samples = ( "$submission_environment"/*.csh )

# endregion Input resolution

# Shell environment and printing ---------------------------------------------

# region Shell support
# Preserve one status for run.csh, remember output directories already handled in this invocation, and clear farm_out at most once.
set CLAS12_SAMPLE_STATUS = 1
set submission_outputs = ()
set farm_cleared = 0
setenv RUNNING_DIR "$cwd"

# This script is intended to be sourced through run.csh, which calls set_environment.csh before reaching this point.
# Validate the shared palette before any colored output so this file never needs to define ANSI escape sequences itself.
if (! $?COLOR_START || ! $?COLOR_ERR || ! $?COLOR_COMPLETION || ! $?COLOR_INFO || ! $?COLOR_WARNING || ! $?COLOR_END) then
    echo "Error: printout colors are unavailable; source the submission workflow through run.csh."
    goto submission_finish
endif

# The protected payload owns the detector commands; this maintained coordinator validates and submits it.
if (! -f "$RUNNING_DIR/src/slurm-submission/external/submit_GEMC_sample.sh") then
    echo "${COLOR_ERR}Error:${COLOR_END} source the submission script from the CLAS12-sample-generator checkout."
    goto submission_finish
endif

setenv SUBMIT_SCRIPT_FILE "$RUNNING_DIR/src/slurm-submission/external/submit_GEMC_sample.sh"

# The check aliases consume check_name/check_path/check_color, print the legacy messages,
# and jump to the corresponding failure label before any dependent stage can run. submission_section prints a consistent heading.
alias submission_dir 'echo "${check_color}--> Checking if ${COLOR_END}${check_name}${check_color} is a directory...${COLOR_END}"; test -d "$check_path"; if ($status != 0) goto submission_missing_dir; printf "${check_color}-->${COLOR_END} %s\n\n" "${COLOR_COMPLETION}${check_name} exists.${COLOR_END}"'
alias submission_file 'echo "${check_color}--> Checking if ${COLOR_END}${check_name}${check_color} is a file...${COLOR_END}"; test -f "$check_path"; if ($status != 0) goto submission_missing_file; printf "${check_color}-->${COLOR_END} %s\n\n" "${COLOR_COMPLETION}${check_name} exists.${COLOR_END}"'
alias submission_section 'echo ""; echo "${COLOR_START}=======================================================================${COLOR_END}"; printf "%s\n" "${COLOR_START}${section}${COLOR_END}"; echo "${COLOR_START}=======================================================================${COLOR_END}"; echo ""'

# endregion Shell support

foreach sample ($samples:q)
    # Resolved sample ---------------------------------------------------------

    # region Resolved sample
    # The helper emits only validated, whitelisted assignments into our private temporary directory.
    source "$sample"
    if ($status != 0) goto submission_finish

    # endregion Resolved sample

    # Setup report and modules ------------------------------------------------

    # region Setup
    # Validate the resolver's control values before they can affect module loading, filesystem cleanup, or Slurm.
    # source selects the report/output conventions; the two Boolean settings must use explicit true/false text.
    # NUM_OF_JOBS and JOB_NEVENTS are positive decimal integers because zero-sized arrays and event limits are invalid.
    if ("$source" != "uniform" && "$source" != "physical") then
        echo "${COLOR_ERR}Error:${COLOR_END} source must be uniform or physical."
        goto submission_finish
    endif

    if ("$CLEAR_FAR_OUT" != "true" && "$CLEAR_FAR_OUT" != "false") goto submission_bad_settings
    if ("$CUSTOM_GEMC_VERSION" != "true" && "$CUSTOM_GEMC_VERSION" != "false") goto submission_bad_settings

    printf '%s\n' "$NUM_OF_JOBS" "$JOB_NEVENTS" | awk '$0 !~ /^[1-9][0-9]*$/ {exit 1}'
    if ($status != 0) goto submission_bad_settings

    # The inherited Slurm export policy must not suppress the configured software environment.
    # GENIE_TUNE preserves the variable name expected by the protected worker while the resolver uses the generator-neutral name.
    setenv SLURM_EXPORT_ENV ALL
    setenv SBATCH_EXPORT ALL
    setenv GENIE_TUNE "$GENERATOR_TUNE"

    # Report the checkout and resolved high-level settings before performing any side effect.
    # Uniform and physical samples retain distinct legacy headings, while both use the same validated values below.
    echo "${COLOR_START}RUNNING_DIR::${COLOR_END} ${RUNNING_DIR}"
    echo ""
    echo
    echo ""
    echo "${COLOR_INFO}///////////////////////////////////////////////////////////////////////${COLOR_END}"
    if ("$source" == "uniform") then
        printf "%s%s%s\n" "${COLOR_INFO}//${COLOR_END}        Setting and submitting uniform sample generation jobs      ${COLOR_INFO}//${COLOR_END}"
    else
        printf "%s%s%s\n" "${COLOR_INFO}//${COLOR_END}        Setting and submitting GENIE sample generation jobs        ${COLOR_INFO}//${COLOR_END}"
    endif
    echo "${COLOR_INFO}///////////////////////////////////////////////////////////////////////${COLOR_END}"
    echo ""
    set section = '= Setup environment variables and paths                               ='
    submission_section

    if ("$source" == "uniform") then
        echo "${COLOR_START}TARGET_VARIATION:${COLOR_END}    ${TARGET_VARIATION}"
        echo
    endif
    echo "${COLOR_START}CLEAR_FAR_OUT:${COLOR_END}       ${CLEAR_FAR_OUT}"
    echo
    echo "${COLOR_START}CUSTOM_GEMC_VERSION:${COLOR_END} ${CUSTOM_GEMC_VERSION}"
    echo
    echo "${COLOR_START}GEMC_VERSION:${COLOR_END}        ${GEMC_VERSION}"
    echo
    echo "${COLOR_START}NUM_OF_JOBS:${COLOR_END}         ${NUM_OF_JOBS}"
    echo
    echo ""
    echo "${COLOR_START}=======================================================================${COLOR_END}"
    if ("$source" == "uniform") then
        printf "%s%s%s\n" "${COLOR_START}= " "Starting uniform generation and submission         ${COLOR_START}=${COLOR_END}"
    else
        printf "%s%s%s\n" "${COLOR_START}= " "Starting GENIE sample generation and submission     ${COLOR_START}=${COLOR_END}"
    endif
    echo "${COLOR_START}=======================================================================${COLOR_END}"
    echo ""

    # Physical conversion keeps its run beneath OUTPATH_BASE; uniform resolution validates that base in the sample report.
    # CLAS12TAGS_DIR is optional and is checked only when a custom clas12Tags tree was explicitly configured.
    set check_color = "$COLOR_START"
    if ("$source" == "physical") then
        echo "${COLOR_START}OUTPATH_BASE: ${COLOR_END}${OUTPATH_BASE}"
        set check_name = OUTPATH_BASE
        set check_path = "$OUTPATH_BASE"
        submission_dir
    endif

    if ("$CLAS12TAGS_DIR" != "") then
        echo "${COLOR_START}CLAS12TAGS_DIR:${COLOR_END} ${CLAS12TAGS_DIR}"
        set check_name = CLAS12TAGS_DIR
        set check_path = "$CLAS12TAGS_DIR"
        submission_dir
    endif

    # CLEAR_FAR_OUT is an invocation-wide maintenance option for ifarm log files, independent of per-sample output cleanup.
    # The guards reject roots, the home directory, this checkout, symlinks, and paths outside a farm_out hierarchy.
    # find removes only regular files directly below the resolved directory; subdirectories and later job logs are preserved.
    set section = '= Handling farm_out directory clearing and custom GEMC version        ='
    submission_section
    if ("$CLEAR_FAR_OUT" == "true" && $farm_cleared == 0) then
        # Limit optional deletion to files directly in the configured user's farm_out directory.
        if ("$farm_out" !~ /* || "$farm_out" == "/" || "$farm_out" == "$HOME" || "$farm_out" == "$RUNNING_DIR" || -l "$farm_out") goto submission_bad_settings
        if (! -d "$farm_out") goto submission_bad_settings

        set resolved_farm = `cd "$farm_out" && pwd -P`
        if ($status != 0 || "$resolved_farm" == "" || "$resolved_farm" == "/" || "$resolved_farm" == "$HOME" || "$RUNNING_DIR" =~ "$resolved_farm"/* || "$resolved_farm" == "$RUNNING_DIR") goto submission_bad_settings
        if ("$resolved_farm" !~ */farm_out && "$resolved_farm" !~ */farm_out/*) goto submission_bad_settings

        echo "${COLOR_START}Clearing farm_out directory...${COLOR_END}"
        echo "${COLOR_START}-----------------------------------------------------------------------${COLOR_END}"
        find "$resolved_farm" -maxdepth 1 -type f -delete
        if ($status != 0) goto submission_finish
        set farm_cleared = 1
        echo
    else if ("$CLEAR_FAR_OUT" == "true") then
        echo "farm_out was already cleared for this submission invocation. Preserve newly created job logs."
        echo
    else
        echo "CLEAR_FAR_OUT$ ${COLOR_START}is set to '${COLOR_END}false${COLOR_START}', skipping farm_out directory clearing...${COLOR_END}"
        echo
    endif

    # Optionally replace the login shell's GEMC module with the requested version.
    # A configured override wins after module loading; otherwise GEMC_DATA_DIR must come from the active environment.
    # Every module or directory failure returns through the shared failure block before output deletion or submission.
    set section = '= Handling custom GEMC version                                        ='
    submission_section
    if ("$CUSTOM_GEMC_VERSION" == "true") then
        echo "${COLOR_START}Loading GEMC version ${COLOR_END}${GEMC_VERSION}${COLOR_START}...${COLOR_END}"
        echo "${COLOR_START}-----------------------------------------------------------------------${COLOR_END}"

        module unload gemc
        if ($status != 0) goto submission_module_failed

        module load gemc/${GEMC_VERSION}
        if ($status != 0) goto submission_module_failed

        echo
        if ("$GEMC_DATA_OVERRIDE" != "") setenv GEMC_DATA_DIR "$GEMC_DATA_OVERRIDE"

        if (! $?GEMC_DATA_DIR) then
            echo "${COLOR_ERR}Error:${COLOR_END} GEMC_DATA_DIR was not set by the loaded environment."
            goto submission_finish
        endif

        echo "${COLOR_START}GEMC_DATA_DIR:${COLOR_END} ${GEMC_DATA_DIR}"
        set check_name = GEMC_DATA_DIR
        set check_path = "$GEMC_DATA_DIR"
        submission_dir
    else
        echo "CUSTOM_GEMC_VERSION$ ${COLOR_START}is set to '${COLOR_END}false${COLOR_START}', skipping custom GEMC version loading...${COLOR_END}"
        echo
    endif

    # Reapply the override when custom module loading is disabled, then validate the final directory in either path.
    if ("$GEMC_DATA_OVERRIDE" != "") setenv GEMC_DATA_DIR "$GEMC_DATA_OVERRIDE"

    if (! $?GEMC_DATA_DIR) then
        echo "${COLOR_ERR}Error:${COLOR_END} GEMC_DATA_DIR is missing; load GEMC or supply --gemc-data-dir."
        goto submission_finish
    endif

    if (! -d "$GEMC_DATA_DIR") then
        echo "${COLOR_ERR}Error:${COLOR_END} GEMC_DATA_DIR is not a directory: $GEMC_DATA_DIR"
        goto submission_finish
    endif

    # Introduce the adapter-specific sample loop report; actual iteration is driven by the resolver-generated sample files.
    if ("$source" == "uniform") then
        set section = '= Looping over particle types                                         ='
    else
        set section = '= Looping over samples                                         ='
    endif

    submission_section

    # endregion Setup

    # Sample report -----------------------------------------------------------

    # region Sample report
    # Purpose: show the resolved scientific and detector identity of this sample before validating or replacing its outputs.
    # Inputs: resolver-generated sample metadata, detector paths, filename prefix, and the shared terminal-color palette.
    # Result: OUTPATH is canonicalized and all required directories/files are confirmed; failures return before submission.
    echo

    # Lead with the source-specific identity a user needs to recognize the selected run.
    # Uniform samples are identified by channel, while physical samples include their target nucleus and generator tune.
    if ("$source" == "uniform") then
        echo "${COLOR_START}Processing particle type ${COLOR_END}${TEMP_OUTPATH_PARTICLE}${COLOR_START} at beam energy ${COLOR_END}${TEMP_BEAM_E}"
    else
        echo "${COLOR_START}Processing GENIE sample for ${COLOR_END}${SAMPLE_TARGET_NUCLEUS}${COLOR_START} (${COLOR_END}${GENERATOR_TUNE}${COLOR_START}) at beam energy ${COLOR_END}${TEMP_BEAM_E}"
    endif
    echo "${COLOR_START}-----------------------------------------------------------------------${COLOR_END}"
    echo

    # Uniform runs report their generated channel and require the shared output base to exist already.
    # Physical runs report the detector target variation here; their full generator provenance follows below.
    if ("$source" == "uniform") then
        echo "${COLOR_START}TEMP_BEAM_E:${COLOR_END} ${TEMP_BEAM_E}"
        echo
        echo "${COLOR_START}TEMP_OUTPATH_PARTICLE:${COLOR_END} ${TEMP_OUTPATH_PARTICLE}"
        echo

        # Sample-specific reports use the informational color supplied by set_environment.csh.
        echo "${COLOR_INFO}OUTPATH_BASE: ${COLOR_END}${OUTPATH_BASE}"
        set check_color = "$COLOR_INFO"
        set check_name = OUTPATH_BASE
        set check_path = "$OUTPATH_BASE"
        submission_dir
        echo "${COLOR_INFO}TEMP_OUTPATH_PARTICLE:${COLOR_END} ${TEMP_OUTPATH_PARTICLE}"
        echo
        echo "${COLOR_INFO}Setting environment variables based on TEMP_BEAM_E and particle type ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
        echo "${COLOR_INFO}-----------------------------------------------------------------------${COLOR_END}"
        echo

    else
        echo "${COLOR_START}TARGET_VARIATION:${COLOR_END} ${TARGET_VARIATION}"
        echo

        # Physical-sample reports use the same shared informational color; field-cage status is still printed explicitly below.
    endif

    echo "${COLOR_INFO}TEMP_BEAM_E_ROUNDED:${COLOR_END} ${TEMP_BEAM_E_ROUNDED}"
    echo

    # The rounded beam label selects detector resources; the exact TEMP_BEAM_E remains part of sample provenance.
    # The physical report keeps the legacy GENIE wording because GENIE is the currently supported physical adapter.
    if ("$source" == "uniform") then
        echo "${COLOR_INFO}Setting paths based on particle type ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
        echo "${COLOR_INFO}-----------------------------------------------------------------------${COLOR_END}"
        echo
    else
        echo "${COLOR_INFO}//////////////////////////////////////////////////////////////////////${COLOR_END}"
        echo "${COLOR_INFO}// Setting GENIE slurm job submission                               //${COLOR_END}"
        echo "${COLOR_INFO}//////////////////////////////////////////////////////////////////////${COLOR_END}"
        echo
        echo "${COLOR_INFO}- Sample parameters ---------------------------------------------------${COLOR_END}"
        echo ""
        echo "${COLOR_INFO}SAMPLE_TARGET_NUCLEUS:${COLOR_END} ${SAMPLE_TARGET_NUCLEUS}"
        echo ""
        echo "${COLOR_INFO}GENIE_TUNE:${COLOR_END} ${GENERATOR_TUNE}"
        echo ""
        echo "${COLOR_INFO}Q2_CUT:${COLOR_END} ${Q2_CUT}"
        echo ""
        echo "${COLOR_INFO}TEMP_BEAM_E:${COLOR_END} ${TEMP_BEAM_E}"
        echo ""
        echo "${COLOR_INFO}FC_STATUS:${COLOR_END} ${FC_STATUS}"
        echo ""
        echo "${COLOR_INFO}FC_STATUS_ENABLED:${COLOR_END} ${FC_STATUS_ENABLED}"
        echo ""
    endif

    # OUTPATH is the completed LUND run directory consumed by this submission and later receives simulation products.
    # Physical conversion may create this directory during setup; uniform generation must have created it beforehand.
    echo "${COLOR_INFO}OUTPATH:${COLOR_END} ${OUTPATH}"
    if ("$source" == "physical") echo ""

    set check_color = "$COLOR_INFO"
    if ("$source" == "physical") set check_color = "$COLOR_START"

    # Enforce the protected worker's conservative path contract before passing any value through its environment.
    # Paths must be absolute, use the allowed character set, and contain no parent-directory traversal components.
    # SAMPLE_FILE_PREFIX is validated separately because it is a filename stem rather than an absolute path.
    foreach path_value ("$OUTPATH" "$GCARD_FILE" "$YAML_FILE" "$SUBMIT_SCRIPT_FILE")
        printf '%s\n' "$path_value" | env LC_ALL=C awk '$0 !~ /^\/[A-Za-z0-9_.\/-]+$/ || $0 ~ /(^|\/)\.\.(\/|$)/ {exit 1}'
        if ($status != 0) goto submission_bad_settings
    end

    printf '%s\n' "$SAMPLE_FILE_PREFIX" | env LC_ALL=C awk '$0 !~ /^[A-Za-z0-9_-][A-Za-z0-9_.-]*$/ {exit 1}'
    if ($status != 0) goto submission_bad_settings

    # Physical input can name a new run directory beneath its validated base, so create it when absent.
    # A missing uniform directory indicates that LUND generation did not complete at the selected location and is fatal.
    if (! -d "$OUTPATH") then
        if ("$source" == "physical") then
            echo "${COLOR_START}--> Checking if ${COLOR_END}OUTPATH${COLOR_START} is a directory...${COLOR_END}"
            printf "%s\n" "${COLOR_START}-->${COLOR_END} ${COLOR_WARNING}Warning:${COLOR_END} the following directory does not exist: ${OUTPATH}"
            printf "%s\n" "${COLOR_START}-->${COLOR_END} ${COLOR_WARNING}Creating OUTPATH.${COLOR_END}"

            mkdir -p "$OUTPATH"
            if ($status != 0) goto submission_finish

            echo "${COLOR_START}----> Checking if ${COLOR_END}OUTPATH${COLOR_START} is a directory...${COLOR_END}"
            printf "%s\n\n" "${COLOR_START}---->${COLOR_END} ${COLOR_COMPLETION}OUTPATH was created successfully.${COLOR_END}"
        else
            set check_name = OUTPATH
            set check_path = "$OUTPATH"
            submission_dir
        endif
    else
        set check_name = OUTPATH
        set check_path = "$OUTPATH"
        submission_dir
    endif

    # Resolve symlinks and relative components before any later recursive output replacement.
    # Reject root, home, the checkout itself, and ancestors of the checkout so cleanup cannot reach project or user data.
    set resolved_out = `cd "$OUTPATH" && pwd -P`
    if ($status != 0 || "$resolved_out" == "" || "$resolved_out" == "/" || "$resolved_out" == "$HOME" || "$resolved_out" == "$RUNNING_DIR" || "$RUNNING_DIR" =~ "$resolved_out"/*) goto submission_bad_settings

    # Each resolver record in one invocation must map to a different run directory.
    # This prevents a later sample from deleting or overwriting simulation outputs prepared for an earlier sample.
    foreach previous_output ($submission_outputs:q)
        if ("$resolved_out" == "$previous_output") then
            echo "${COLOR_ERR}Error:${COLOR_END} samples in one invocation must use distinct OUTPATH directories."
            goto submission_finish
        endif
    end

    set submission_outputs = ($submission_outputs:q "$resolved_out")

    # Export the canonical path so the protected Slurm worker and all subsequent checks use the same directory identity.
    setenv OUTPATH "$resolved_out"

    # Physical reports retain the torus setting and checkout check used by the archived submission workflow.
    # Uniform and physical paths converge below on the same detector-resource validation.
    if ("$source" == "physical") then
        echo "${COLOR_INFO}- Job parameters ------------------------------------------------------${COLOR_END}"
        echo ""
        echo "${COLOR_INFO}TORUS_FIELD:${COLOR_END} ${TORUS_FIELD}"
        echo ""
        set check_name = RUNNING_DIR
        set check_path = "$RUNNING_DIR"
        submission_dir
    endif

    # REQUIREMENTS_PATH groups the reviewed GCARD and YAML selected for this beam energy, target variation, and GEMC version.
    echo "${COLOR_INFO}REQUIREMENTS_PATH:${COLOR_END} ${REQUIREMENTS_PATH}"
    echo

    set check_color = "$COLOR_INFO"
    set check_name = REQUIREMENTS_PATH
    set check_path = "$REQUIREMENTS_PATH"
    submission_dir

    # Report the automatic detector selection before checking its two concrete inputs.
    # GCARD controls GEMC geometry and fields; YAML controls the downstream CLAS12 reconstruction configuration.
    if ("$source" == "uniform") then
        echo "${COLOR_INFO}Setting GCARD_FILE and YAML_FILE files based on TEMP_BEAM_E and TARGET_VARIATION ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
    else
        echo "${COLOR_INFO}Setting GCARD_FILE and YAML_FILE files based on TEMP_BEAM_E and TARGET_VARIATION${COLOR_END}"
    endif
    echo "${COLOR_INFO}-----------------------------------------------------------------------${COLOR_END}"
    echo

    # Reuse the file-check alias so either missing detector input follows the same colored error and return path.
    foreach check_name (GCARD_FILE YAML_FILE)
        if ("$check_name" == "GCARD_FILE") set check_path = "$GCARD_FILE"
        if ("$check_name" == "YAML_FILE") set check_path = "$YAML_FILE"
        echo "${COLOR_INFO}${check_name}:${COLOR_END} ${check_path}"
        submission_file
    end

    # endregion Sample report

    # Validate LUND and prepare outputs ---------------------------------------

    # region Output preparation
    # Purpose: prove that the complete worker input set is usable before replacing prior simulation products.
    # Inputs: the resolved file prefix and job count, active software environment, source type, and canonical OUTPATH.
    # Outputs: fresh empty simulation directories; LUND inputs and detector configuration remain untouched.

    # Array tasks are numbered from 1 through NUM_OF_JOBS, matching the protected worker's SLURM_ARRAY_TASK_ID lookup.
    # Require every corresponding LUND file to be nonempty so a submitted task cannot start without its event input.
    @ index = 1
    while ($index <= $NUM_OF_JOBS)
        if (! -s "${OUTPATH}/lundfiles/${SAMPLE_FILE_PREFIX}_${index}.txt") then
            echo "${COLOR_ERR}Error:${COLOR_END} missing or empty LUND input: ${OUTPATH}/lundfiles/${SAMPLE_FILE_PREFIX}_${index}.txt"
            goto submission_finish
        endif
        @ index ++
    end

    # Confirm the login shell exposes both the scheduler command and the two executables used by the worker payload.
    # This catches incomplete module/environment setup before any existing output directory is removed.
    foreach executable (sbatch gemc recon-util)
        which "$executable" >& /dev/null
        if ($status != 0) then
            echo "${COLOR_ERR}Error:${COLOR_END} $executable is unavailable in the loaded environment."
            goto submission_finish
        endif
    end

    # GEMC and reconstruction always produce mchipo and reconhipo directories.
    # Uniform runs additionally replace rootfiles to preserve their archived output layout and monitoring workflow.
    if ("$source" == "uniform") then
        echo "${COLOR_INFO}Setting output directory structure ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
        set output_dirs = (mchipo reconhipo rootfiles)
    else
        echo "${COLOR_INFO}Setting output directory structure${COLOR_END}"
        set output_dirs = (mchipo reconhipo)
    endif

    echo "${COLOR_INFO}-----------------------------------------------------------------------${COLOR_END}"
    echo

    # Reject symbolic-link destinations before recursive deletion, even though OUTPATH itself was canonicalized above.
    # This keeps replacement confined to real child directories of the validated run directory.
    foreach directory ($output_dirs)
        if (-l "$OUTPATH/$directory") goto submission_bad_settings
    end

    # Submission intentionally starts a clean simulation attempt: remove only the source-specific directories listed above.
    # The completed OUTPATH/lundfiles directory is never included and therefore remains the immutable job input.
    echo "${COLOR_INFO}Removing old directory structure for MC simulation here...${COLOR_END}"

    foreach directory ($output_dirs)
        rm -rf -- "$OUTPATH/$directory"
        if ($status != 0) goto submission_finish
    end
    echo

    # Recreate each directory explicitly and stop at the first failure so Slurm never receives a partial output layout.
    echo "${COLOR_INFO}Setting up directory structure for MC simulation here...${COLOR_END}"
    foreach directory ($output_dirs)
        mkdir "$OUTPATH/$directory"
        if ($status != 0) goto submission_finish
    end
    echo

    # Print a final pre-submission inventory: LUND should be populated, while the recreated simulation directories are empty.
    echo "${COLOR_INFO}Number of files in target directory (OUTPATH):${COLOR_END}"
    echo "${COLOR_INFO}Number of lund files:     \t\t${COLOR_END} `ls ${OUTPATH}/lundfiles | wc -l`"
    echo "${COLOR_INFO}Number of mchipo files:   \t\t${COLOR_END} `ls ${OUTPATH}/mchipo | wc -l`"
    echo "${COLOR_INFO}Number of reconhipo files:\t\t${COLOR_END} `ls ${OUTPATH}/reconhipo | wc -l`"
    echo

    # endregion Output preparation

    # Slurm handoff -----------------------------------------------------------

    # region Submission
    # Purpose: expose the exact Slurm array request, validate the protected worker path, and hand the job to ifarm.
    # The source-specific text is informational; uniform and physical samples share the same sbatch command contract.
    if ("$source" == "uniform") then
        echo "${COLOR_INFO}Submitting sbatch job for BeamE = ${COLOR_END}${TEMP_BEAM_E}"
        echo "${COLOR_INFO}-----------------------------------------------------------------------${COLOR_END}"
        echo
    else
        echo "${COLOR_INFO}- Submitting jobs ------------------------------------------------------${COLOR_END}"
        echo ""
        echo "${COLOR_INFO}Submitting GENIE sbatch job...${COLOR_END}"
    endif

    echo "${COLOR_INFO}SLURM_JOB_NAME:${COLOR_END} ${SLURM_JOB_NAME}"
    echo ""

    # ARRAY maps one Slurm task to each validated PREFIX_INDEX.txt input, using the same inclusive range checked above.
    setenv ARRAY 1-${NUM_OF_JOBS}
    echo "${COLOR_INFO}ARRAY:${COLOR_END} ${ARRAY}"
    echo ""

    echo "${COLOR_INFO}SUBMIT_SCRIPT_FILE:${COLOR_END} ${SUBMIT_SCRIPT_FILE}"
    set check_name = SUBMIT_SCRIPT_FILE
    set check_path = "$SUBMIT_SCRIPT_FILE"
    submission_file

    # Echo the effective command for provenance and troubleshooting before invoking the scheduler.
    # The protected payload receives all previously exported sample, detector, and output variables through Slurm's environment.
    echo "${COLOR_INFO}Submitted job with command:${COLOR_END}"
    echo "${COLOR_INFO}sbatch --job-name=${COLOR_END}${SLURM_JOB_NAME}${COLOR_INFO} --array=${COLOR_END}${ARRAY} ${SUBMIT_SCRIPT_FILE}"

    # A scheduler rejection is fatal for this invocation and prevents later resolved samples from being submitted silently.
    sbatch --job-name="$SLURM_JOB_NAME" --array="$ARRAY" "$SUBMIT_SCRIPT_FILE"
    if ($status != 0) goto submission_sbatch_failed
    echo
    echo

    # endregion Submission
end

# Reaching the end of every resolved sample means all requested arrays were accepted by sbatch.
set CLAS12_SAMPLE_STATUS = 0
goto submission_finish

# Sourced-shell failure and return --------------------------------------------

# region Return
# Purpose: funnel every validation, environment, and scheduler failure through one cleanup path.
# Inputs: CLAS12_SAMPLE_STATUS carries the public result; check_name/check_path/check_color describe alias failures.
# Result: temporary resolver data and local aliases are removed, then the status is returned without exiting the caller's shell.

# Directory and file aliases populate the shared check variables immediately before jumping to these labels.
submission_missing_dir:
printf "%s\n" "${check_color}-->${COLOR_END} ${COLOR_ERR}Error:${COLOR_END} the following directory does not exist: ${check_path}"
goto submission_finish

submission_missing_file:
printf "%s\n" "${check_color}-->${COLOR_END} ${COLOR_ERR}Error:${COLOR_END} the following file does not exist: ${check_path}"
goto submission_finish

# Invalid values and unsafe paths deliberately share one message because both require correcting resolved submission input.
submission_bad_settings:
echo "${COLOR_ERR}Error:${COLOR_END} invalid setting or unsafe path; check the submission config or CLI settings."
goto submission_finish

# Module failure prevents later samples from inheriting a partially changed GEMC environment.
submission_module_failed:
echo "${COLOR_ERR}Error:${COLOR_END} GEMC module setup failed; no subsequent sample was submitted."
goto submission_finish

# An unsuccessful sbatch call also stops the multi-sample loop; falling through reaches the common cleanup below.
submission_sbatch_failed:
echo "${COLOR_ERR}Error:${COLOR_END} sbatch failed; no subsequent sample was submitted."

submission_finish:

# Only remove the private mktemp directory created during input resolution, never a user-supplied path.
if ("$submission_environment" != "") then
    rm -rf -- "$submission_environment"
endif

# Discard helper aliases so sourcing this workflow does not pollute the interactive login shell.
unalias submission_dir submission_file submission_section

# Execute a harmless child shell with the chosen code: tcsh adopts that command's status while the user's shell remains alive.
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"

# endregion Return
