#!/bin/tcsh

#
# Created by Alon Sportes on 19/09/2026.
#

# Setup and submit ------------------------------------------------------------
# Description:
#	validates resolved sample settings and submits one GEMC/reconstruction Slurm array for each selected sample.
# 
# Purpose:
#	provide the single ifarm setup/submission path for completed uniform or physical LUND samples.
# 
# Usage:
#	source run.csh --workflow submit --lund-dir RUN/lundfiles [--config FILE] [overrides].
# 
# Workflow:
#   1. Ask resolve_inputs.py to validate configuration and emit private tcsh assignments.
#   2. Load the requested GEMC environment and report the resolved detector and sample inputs.
#   3. Validate every LUND file and worker command before replacing simulation output directories.
#   4. Submit the protected GEMC/reconstruction payload as a Slurm array and return its status.
# 
# Submission options resolved before this script acts:
#   --execute                     Submit and replace simulation outputs; default is preview only.
#   --config FILE                 Read optional key = value submission settings.
#   --lund-dir DIRECTORY          Select completed RUN/lundfiles input; repeat for several samples.
#   --source uniform|physical     Override or confirm manifest workflow metadata.
#   --beam-energy GeV             Override or confirm truth beam energy.
#   --rgm-target ID               Override or confirm truth target identity.
#   --channel/--hadron/--hadron-region VALUE  Describe uniform sample content.
#   --event-generator/--tune/--q2-cut VALUE   Describe physical-input provenance.
#   --prefix NAME                 Set the LUND filename stem when no manifest supplies it.
#   --gemc-version VERSION        Select the GEMC module/version (fallback: 5.14).
#   --gemc-target-variation NAME  Select the detector target variation.
#   --gcard FILE / --yaml FILE    Override detector and reconstruction inputs.
#   --torus SCALE                 Override the beam-dependent torus scale.
#   --num-jobs N                  Submit the first N completed files (default: all).
#   --events-per-job N            Set the shared worker event limit.
#   --job-name NAME               Override the derived Slurm job name.
#   --clas12tags-dir DIRECTORY    Use a custom clas12Tags checkout as GEMC_DATA_DIR.
#   --clear-farm-out true|false / --farm-out DIRECTORY  Control guarded farm log cleanup.
#   --fc-status 0|1               Set the legacy physical report/filename label only.
#   --help                        Print submission help without replacing outputs or submitting.
# 
# Inputs:
#	manifest/config/CLI settings (GEMC fallback 5.14), existing OUTPATH/lundfiles/PREFIX_INDEX.txt, GCARD, YAML, and the preloaded ifarm GEMC environment.
# 
# Outputs:
#	Slurm jobs writing OUTPATH/mchipo and OUTPATH/reconhipo; uniform samples also recreate rootfiles.
# 
# Failure behavior:
#	checked failures jump to the shared return block; sourcing never exits the user's login shell.
# 
# Printing:
#	run.csh sources set_environment.csh first; that file owns the COLOR_* environment-variable palette used below.
# 
# Preview is the default:
#	validate/load the environment and print commands without changing sample outputs.
# 
# WARNING:
#	--execute submits jobs and replaces the selected sample's simulation output directories. LUND is preserved.
#	All paths must be absolute and contain only letters, numbers, /, _, -, and . (protected payload contract).
#	JOB_NEVENTS is an event limit shared by the array, not a claim about each input file's length.

# Terminal color initialization -----------------------------------------------

if ( -f ./src/launcher/environment/set_banners.csh ) then
    source ./src/launcher/environment/set_banners.csh
else
    echo "\033[31mError:\033[0m the following file does not exist: ./src/launcher/environment/set_banners.csh\n"
    exit 1
endif

set banner_title = "Running Slurm submission script"
set banner_color = "$COLOR_START"
code_banner
echo ""

# Input resolution ------------------------------------------------------------

# region Input resolution
# Python only resolves and validates data. Preloaded environment handling and sbatch remain in this sourced login shell.
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

# Create a unique private directory for the resolver's per-sample assignment files. mktemp replaces
# the X template with a collision-resistant suffix and creates the directory atomically; the common
# submission_finish path removes it. A failed command or empty result cannot be used safely, so stop
# before invoking the resolver or performing any submission side effect.
set submission_environment = `mktemp -d /tmp/clas12-submit.XXXXXXXX`
if ($status != 0 || "$submission_environment" == "") goto submission_finish

# Resolve every CLI/config/manifest input before the shell performs setup or submission. The private
# --environment-dir receives one validated tcsh assignment file per selected --lund-dir; $argv:q passes
# the original arguments while preserving each argument as one quoted value. A resolver error stops here,
# so the shared cleanup path removes the temporary directory before any outputs are replaced or jobs submitted.
python3 src/slurm-submission/resolve_inputs.py --environment-dir "$submission_environment" $argv:q
if ($status != 0) goto submission_finish

# The resolver writes one numbered assignment file for each --lund-dir, after validating every
# selected sample. This list drives the loop below: each file configures one sample and therefore
# produces one independent Slurm array. A later failure stops the loop without cancelling arrays
# that were already accepted by Slurm.
set samples = ( "$submission_environment"/*.csh )

# endregion Input resolution

# Shell environment and printing ---------------------------------------------

# region Shell support
# Preserve one status for run.csh, remember output directories already handled in this invocation, and clear farm_out at most once.
# Every maintained export clears both tcsh namespaces first. Without `unset`, a stale local value
# can shadow the new `setenv` value; without `unsetenv`, inherited state survives until assignment.
set CLAS12_SAMPLE_STATUS = 1
set submission_outputs = ()
set farm_cleared = 0
unset RUNNING_DIR
unsetenv RUNNING_DIR
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

unset SUBMIT_SCRIPT_FILE
unsetenv SUBMIT_SCRIPT_FILE
setenv SUBMIT_SCRIPT_FILE "$RUNNING_DIR/src/slurm-submission/external/submit_GEMC_sample.sh"

# The check aliases consume check_name/check_path/check_color, print the legacy messages,
# and jump to the corresponding failure label before any dependent stage can run. code_subbanner wraps the shared banner renderer.
alias submission_dir 'echo "${check_color}--> Checking if ${COLOR_END}${check_name}${check_color} is a directory...${COLOR_END}"; test -d "$check_path"; if ($status != 0) goto submission_missing_dir; echo "${check_color}-->${COLOR_END} ${COLOR_COMPLETION}${check_name} exists.${COLOR_END}\n"'
alias submission_file 'echo "${check_color}--> Checking if ${COLOR_END}${check_name}${check_color} is a file...${COLOR_END}"; test -f "$check_path"; if ($status != 0) goto submission_missing_file; echo "${check_color}-->${COLOR_END} ${COLOR_COMPLETION}${check_name} exists.${COLOR_END}"'

# endregion Shell support

foreach sample ($samples:q)
    # Resolved sample ---------------------------------------------------------

    # region Resolved sample
    # The helper emits only validated, whitelisted assignments into our private temporary directory.
    source "$sample"
    if ($status != 0) goto submission_finish
    if ("$SUBMISSION_EXECUTE" == "false") then
        echo "${COLOR_INFO}PREVIEW:${COLOR_END}\nNo sbatch, output replacement or farm_out cleanup; add --execute to submit."
        echo ""
    endif

    # endregion Resolved sample

    # Setup report and modules ------------------------------------------------

    # region Setup
    # Validate the resolver's control values before they can affect filesystem cleanup or Slurm.
    # source selects the report/output conventions; the cleanup Boolean must use explicit true/false text.
    # NUM_OF_JOBS and JOB_NEVENTS are positive decimal integers because zero-sized arrays and event limits are invalid.
    if ("$source" != "uniform" && "$source" != "physical") then
        echo "${COLOR_ERR}Error:${COLOR_END} source must be uniform or physical."
        goto submission_finish
    endif

    if ("$CLEAR_FAR_OUT" != "true" && "$CLEAR_FAR_OUT" != "false") goto submission_bad_settings
    printf '%s\n' "$NUM_OF_JOBS" "$JOB_NEVENTS" | awk '$0 !~ /^[1-9][0-9]*$/ {exit 1}'
    if ($status != 0) goto submission_bad_settings

    # The inherited Slurm export policy must not suppress the configured software environment.
    # GENIE_TUNE preserves the variable name expected by the protected worker while the resolver uses the generator-neutral name.
    unset SLURM_EXPORT_ENV
    unsetenv SLURM_EXPORT_ENV
    setenv SLURM_EXPORT_ENV ALL
    unset SBATCH_EXPORT
    unsetenv SBATCH_EXPORT
    setenv SBATCH_EXPORT ALL
    unset GENIE_TUNE
    unsetenv GENIE_TUNE
    setenv GENIE_TUNE "$GENERATOR_TUNE"

    # Report the checkout and resolved high-level settings before performing any side effect.
    # Uniform and physical samples retain distinct legacy headings, while both use the same validated values below.
    set subbanner_title = "Setup environment variables and paths"
    set subbanner_color = "$COLOR_START"
    code_subbanner

    echo "${COLOR_START}RUNNING_DIR:${COLOR_END}         ${RUNNING_DIR}"
    if ("$source" == "uniform") then
        echo "${COLOR_START}TARGET_VARIATION:${COLOR_END}    ${TARGET_VARIATION}"
    endif
    echo "${COLOR_START}CLEAR_FAR_OUT:${COLOR_END}       ${CLEAR_FAR_OUT}"
    echo "${COLOR_START}GEMC_VERSION:${COLOR_END}        ${GEMC_VERSION}"
    echo "${COLOR_START}NUM_OF_JOBS:${COLOR_END}         ${NUM_OF_JOBS}"
    if ("$source" == "uniform") then
		echo "${COLOR_START}Sample type:${COLOR_INFO} 	     uniform${COLOR_END}"
    else
        echo "${COLOR_START}Sample type:${COLOR_INFO}	     physical (GENIE)${COLOR_END}"
    endif
    echo ""

    # CLAS12TAGS_DIR selects an optional custom fork of https://github.com/gemc/clas12Tags, for example when testing target geometry.
    set check_color = "$COLOR_START"
    if ("$CLAS12TAGS_DIR" != "") then
        echo "${COLOR_START}CLAS12TAGS_DIR:${COLOR_END} ${CLAS12TAGS_DIR}"
        set check_name = CLAS12TAGS_DIR
        set check_path = "$CLAS12TAGS_DIR"
        submission_dir
    endif

    # CLEAR_FAR_OUT is an invocation-wide maintenance option for ifarm log files, independent of per-sample output cleanup.
    # The guards reject roots, the home directory, this checkout, symlinks, and paths outside a farm_out hierarchy.
    # find removes only regular files directly below the resolved directory; subdirectories and later job logs are preserved.
    set subbanner_title = "Handling farm_out directory clearing and GEMC data"
    set subbanner_color = "$COLOR_START"
    code_subbanner
    if ("$CLEAR_FAR_OUT" == "true" && $farm_cleared == 0) then
        # Limit optional deletion to files directly in the configured user's farm_out directory.
        if ("$farm_out" !~ /* || "$farm_out" == "/" || "$farm_out" == "$HOME" || "$farm_out" == "$RUNNING_DIR" || -l "$farm_out") goto submission_bad_settings
        if (! -d "$farm_out") goto submission_bad_settings

        set resolved_farm = `cd "$farm_out" && pwd -P`
        if ($status != 0 || "$resolved_farm" == "" || "$resolved_farm" == "/" || "$resolved_farm" == "$HOME" || "$RUNNING_DIR" =~ "$resolved_farm"/* || "$resolved_farm" == "$RUNNING_DIR") goto submission_bad_settings
        if ("$resolved_farm" !~ */farm_out && "$resolved_farm" !~ */farm_out/*) goto submission_bad_settings

        set banner_title = "Clearing farm_out directory"
        set subbanner_color = "$COLOR_START"
        code_banner
        if ("$SUBMISSION_EXECUTE" == "true") then
            find "$resolved_farm" -maxdepth 1 -type f -delete
            if ($status != 0) goto submission_finish
        else
            echo "PREVIEW: would clear files in $resolved_farm"
        endif
        set farm_cleared = 1
        echo
    else if ("$CLEAR_FAR_OUT" == "true") then
        echo "farm_out was already cleared for this submission invocation. Preserve newly created job logs."
        echo
    else
        echo "CLEAR_FAR_OUT$ ${COLOR_START}is set to '${COLOR_END}false${COLOR_START}', skipping farm_out directory clearing...${COLOR_END}"
        echo
    endif

    # GEMC is already loaded on ifarm, and its environment supplies the standard GEMC_DATA_DIR.
    # A configured CLAS12TAGS_DIR replaces that value with a custom clas12Tags checkout, such as
    # a fork used to test target geometry. Directory failures return before deletion or submission.
    set subbanner_title = "Checking preloaded GEMC data"
    set subbanner_color = "$COLOR_START"
    code_subbanner

    # Selecting a custom clas12Tags checkout means every submitted job must use that fork.
    if ("$CLAS12TAGS_DIR" != "") then
        unset GEMC_DATA_DIR
        unsetenv GEMC_DATA_DIR
        setenv GEMC_DATA_DIR "$CLAS12TAGS_DIR"
    endif

    if (! $?GEMC_DATA_DIR) then
        echo "${COLOR_ERR}Error:${COLOR_END} GEMC_DATA_DIR is missing from the preloaded GEMC environment; select --clas12tags-dir for a custom checkout."
        goto submission_finish
    endif

    echo "${COLOR_START}GEMC_DATA_DIR:${COLOR_END} ${GEMC_DATA_DIR}"
    set check_name = GEMC_DATA_DIR
    set check_path = "$GEMC_DATA_DIR"
    submission_dir
    # endregion Setup

    # Sample report -----------------------------------------------------------

    # region Sample report
    # Purpose:
    #   show the resolved scientific and detector identity of this sample before validating or replacing its outputs.
    #
    # Inputs:
    #   resolver-generated sample metadata, detector paths, filename prefix, and the shared terminal-color palette.
    # 
    # Result: OUTPATH is canonicalized and all required directories/files are confirmed; failures return before submission.

    # Lead with the source-specific identity a user needs to recognize the selected run.
    # Uniform samples are identified by channel, while physical samples include their target nucleus and generator tune.
    if ("$source" == "uniform") then
        set subbanner_title = "Uniform sample job parameters"
    else
        set subbanner_title = "GENIE sample job parameters"
    endif
    set subbanner_color = "$COLOR_START"
    code_subbanner

    # Describe the selected sample. UNIFORM_SAMPLE_CHANNEL identifies the generated uniform final state,
    # such as 1e or enFD, and BEAM_ENERGY_LABEL is its canonical MeV label, such as 2070MeV.
    # Physical samples have no uniform channel, so they report the GEMC target variation instead.
    if ("$source" == "uniform") then
        echo "${COLOR_START}UNIFORM_SAMPLE_CHANNEL:${COLOR_END} ${UNIFORM_SAMPLE_CHANNEL}"
    endif

    # DETECTOR_ENERGY_GROUP is derived automatically from BEAM_ENERGY_LABEL and selects the matching
    # detector-resource directory: 2070MeV -> 2GeV, 4029MeV -> 4GeV, or 5986MeV -> 6GeV.
    echo "${COLOR_START}BEAM_ENERGY_LABEL:${COLOR_END}      ${BEAM_ENERGY_LABEL}"
    echo "${COLOR_START}TARGET_VARIATION:${COLOR_END}       ${TARGET_VARIATION}"
    echo "${COLOR_START}DETECTOR_ENERGY_GROUP:${COLOR_END}  ${DETECTOR_ENERGY_GROUP}"
    echo "${COLOR_START}TORUS_FIELD:${COLOR_END}            ${TORUS_FIELD}"
    echo

    # Introduce the source-specific path report.
    if ("$source" == "uniform") then
        set subbanner_title = "Setting paths for channel ${UNIFORM_SAMPLE_CHANNEL}"
        set subbanner_color = "$COLOR_START"
        code_subbanner
    else
        set subbanner_title = "Setting GENIE Slurm job submission"
        set subbanner_color = "$COLOR_START"
        code_subbanner
        echo
        set subbanner_title = "Sample parameters"
        set subbanner_color = "$COLOR_START"
        code_subbanner
        echo ""
        echo "${COLOR_START}SAMPLE_TARGET_NUCLEUS:${COLOR_END} ${SAMPLE_TARGET_NUCLEUS}"
        echo "${COLOR_START}GENIE_TUNE:${COLOR_END} ${GENERATOR_TUNE}"
        echo "${COLOR_START}Q2_CUT:${COLOR_END} ${Q2_CUT}"
        echo "${COLOR_START}BEAM_ENERGY_LABEL:${COLOR_END} ${BEAM_ENERGY_LABEL}"
        echo "${COLOR_START}FC_STATUS:${COLOR_END} ${FC_STATUS}"
        echo "${COLOR_START}FC_STATUS_ENABLED:${COLOR_END} ${FC_STATUS_ENABLED}"
    endif

    # OUTPATH is the completed LUND run directory consumed by this submission and later receives simulation products.
    # Physical conversion may create this directory during setup; uniform generation must have created it beforehand.
    echo "${COLOR_START}OUTPATH:${COLOR_END} ${OUTPATH}"
    if ("$source" == "physical") echo ""

    set check_color = "$COLOR_START"
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
            echo "${COLOR_START}-->${COLOR_END} ${COLOR_WARNING}Warning:${COLOR_END} the following directory does not exist: ${OUTPATH}"
            echo "${COLOR_START}-->${COLOR_END} ${COLOR_WARNING}Creating OUTPATH.${COLOR_END}"

            # Resolution normally requires this directory already; fail if it disappeared during setup.
            echo "${COLOR_ERR}Error:${COLOR_END} resolved LUND run directory disappeared: $OUTPATH"
            goto submission_finish

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
    unset OUTPATH
    unsetenv OUTPATH
    setenv OUTPATH "$resolved_out"

    # Retain the archived physical-workflow checkout check. Both source types converge immediately
    # afterward on the same detector-resource validation and use the same resolved TORUS_FIELD.
    if ("$source" == "physical") then
        set check_name = RUNNING_DIR
        set check_path = "$RUNNING_DIR"
        submission_dir
    endif

    # REQUIREMENTS_DIR groups the reviewed GCARD and YAML selected for this beam energy, target variation, and GEMC version.
    echo "${COLOR_START}REQUIREMENTS_DIR:${COLOR_END} ${REQUIREMENTS_DIR}"
    set check_color = "$COLOR_START"
    set check_name = REQUIREMENTS_DIR
    set check_path = "$REQUIREMENTS_DIR"
    submission_dir

    # Reuse the file-check alias so either missing detector input follows the same colored error and return path.
    foreach check_name (GCARD_FILE YAML_FILE)
        if ("$check_name" == "GCARD_FILE") set check_path = "$GCARD_FILE"
        if ("$check_name" == "YAML_FILE") set check_path = "$YAML_FILE"
        echo "${COLOR_START}${check_name}:${COLOR_END} ${check_path}"
        submission_file
    end
    echo

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
    set required_executables = (gemc recon-util)
    if ("$SUBMISSION_EXECUTE" == "true") set required_executables = (sbatch $required_executables)
    foreach executable ($required_executables)
        which "$executable" >& /dev/null
        if ($status != 0) then
            echo "${COLOR_ERR}Error:${COLOR_END} $executable is unavailable in the loaded environment."
            goto submission_finish
        endif
    end

    # GEMC and reconstruction always produce mchipo and reconhipo directories.
    # Uniform runs additionally replace rootfiles to preserve their archived output layout and monitoring workflow.
    if ("$source" == "uniform") then
        set subbanner_title = "Setting output directories for ${UNIFORM_SAMPLE_CHANNEL}"
        set output_dirs = (mchipo reconhipo rootfiles)
    else
        set subbanner_title = "Setting output directories"
        set output_dirs = (mchipo reconhipo)
    endif
    set subbanner_color = "$COLOR_START"
    code_subbanner
    echo

    # Reject symbolic-link destinations before recursive deletion, even though OUTPATH itself was canonicalized above.
    # This keeps replacement confined to real child directories of the validated run directory.
    foreach directory ($output_dirs)
        if (-l "$OUTPATH/$directory") goto submission_bad_settings
    end

    # Submission intentionally starts a clean simulation attempt: remove only the source-specific directories listed above.
    # The completed OUTPATH/lundfiles directory is never included and therefore remains the immutable job input.
    if ("$SUBMISSION_EXECUTE" == "true") then
        echo "${COLOR_START}Removing old directory structure for MC simulation here...${COLOR_END}"

        foreach directory ($output_dirs)
            rm -rf -- "$OUTPATH/$directory"
            if ($status != 0) goto submission_finish
        end
        echo

        # Recreate each directory explicitly and stop at the first failure so Slurm never receives a partial output layout.
        echo "${COLOR_START}Setting up directory structure for MC simulation here...${COLOR_END}"
        foreach directory ($output_dirs)
            mkdir "$OUTPATH/$directory"
            if ($status != 0) goto submission_finish
        end
        echo

    else
        echo "PREVIEW: would replace $output_dirs under $OUTPATH; existing outputs are preserved."
    endif

    # Print a final pre-submission inventory: LUND should be populated, while the recreated simulation directories are empty.
    echo "${COLOR_START}Number of files in target directory (OUTPATH):${COLOR_END}"
    echo "${COLOR_START}Number of lund files:     \t\t${COLOR_END} `ls ${OUTPATH}/lundfiles | wc -l`"
    if ("$SUBMISSION_EXECUTE" == "true") then
        echo "${COLOR_START}Number of mchipo files:   \t\t${COLOR_END} `ls ${OUTPATH}/mchipo | wc -l`"
        echo "${COLOR_START}Number of reconhipo files:\t\t${COLOR_END} `ls ${OUTPATH}/reconhipo | wc -l`"
    else
        set mc_count = 0
        set reco_count = 0
        if (-d "$OUTPATH/mchipo") set mc_count = `ls ${OUTPATH}/mchipo | wc -l`
        if (-d "$OUTPATH/reconhipo") set reco_count = `ls ${OUTPATH}/reconhipo | wc -l`
        echo "${COLOR_START}Number of mchipo files:   \t\t${COLOR_END} ${mc_count}"
        echo "${COLOR_START}Number of reconhipo files:\t\t${COLOR_END} ${reco_count}"
    endif
    echo

    # endregion Output preparation

    # Slurm handoff -----------------------------------------------------------

    # region Submission
    # Purpose: expose the exact Slurm array request, validate the protected worker path, and hand the job to ifarm.
    # The source-specific text is informational; uniform and physical samples share the same sbatch command contract.
    if ("$source" == "uniform") then
        set subbanner_title = "Submitting sbatch job for beam energy ${BEAM_ENERGY_LABEL}"
    else
        set subbanner_title = "Submitting GENIE sbatch job"
    endif
    set subbanner_color = "$COLOR_START"
    code_subbanner
    echo

    echo "${COLOR_START}SLURM_JOB_NAME:${COLOR_END} ${SLURM_JOB_NAME}"
    echo ""

    # ARRAY maps one Slurm task to each validated PREFIX_INDEX.txt input, using the same inclusive range checked above.
    unset ARRAY
    unsetenv ARRAY
    setenv ARRAY 1-${NUM_OF_JOBS}
    echo "${COLOR_START}ARRAY:${COLOR_END} ${ARRAY}"
    echo ""

    echo "${COLOR_START}SUBMIT_SCRIPT_FILE:${COLOR_END} ${SUBMIT_SCRIPT_FILE}"
    set check_name = SUBMIT_SCRIPT_FILE
    set check_path = "$SUBMIT_SCRIPT_FILE"
    submission_file

    # Echo the effective command for provenance and troubleshooting before invoking the scheduler.
    # The protected payload receives all previously exported sample, detector, and output variables through Slurm's environment.
    if ("$SUBMISSION_EXECUTE" == "true") then
        echo "${COLOR_START}Submitted job with command:${COLOR_END}"
    else
        echo "${COLOR_START}Preview command (not submitted):${COLOR_END}"
    endif
    echo "${COLOR_START}sbatch --job-name=${COLOR_END}${SLURM_JOB_NAME}${COLOR_START} --array=${COLOR_END}${ARRAY} ${SUBMIT_SCRIPT_FILE}"

    # A scheduler rejection is fatal for this invocation and prevents later resolved samples from being submitted silently.
    if ("$SUBMISSION_EXECUTE" == "true") then
        sbatch --job-name="$SLURM_JOB_NAME" --array="$ARRAY" "$SUBMIT_SCRIPT_FILE"
        if ($status != 0) goto submission_sbatch_failed
    endif
    echo
    echo

    # endregion Submission
end

# All samples were successfully previewed, or all requested arrays were accepted with --execute.
set CLAS12_SAMPLE_STATUS = 0
goto submission_finish

# Sourced-shell failure and return --------------------------------------------

# region Return
# Purpose: funnel every validation, environment, and scheduler failure through one cleanup path.
# Inputs: CLAS12_SAMPLE_STATUS carries the public result; check_name/check_path/check_color describe alias failures.
# Result: temporary resolver data and local aliases are removed, then the status is returned without exiting the caller's shell.

# Directory and file aliases populate the shared check variables immediately before jumping to these labels.
submission_missing_dir:
echo "${check_color}-->${COLOR_END} ${COLOR_ERR}Error:${COLOR_END} the following directory does not exist: ${check_path}"
goto submission_finish

submission_missing_file:
echo "${check_color}-->${COLOR_END} ${COLOR_ERR}Error:${COLOR_END} the following file does not exist: ${check_path}"
goto submission_finish

# Invalid values and unsafe paths deliberately share one message because both require correcting resolved submission input.
submission_bad_settings:
echo "${COLOR_ERR}Error:${COLOR_END} invalid setting or unsafe path; check the submission config or CLI settings."
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
unalias code_banner code_subbanner submission_dir submission_file

# Execute a harmless child shell with the chosen code: tcsh adopts that command's status while the user's shell remains alive.
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"

# endregion Return
