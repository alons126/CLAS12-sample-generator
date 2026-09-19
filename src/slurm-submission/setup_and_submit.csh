#!/bin/tcsh

#
# Created by Alon Sportes on 19/09/2026.
#

# Setup and submit ------------------------------------------------------------
# Purpose: the single ifarm setup/submission path for completed uniform or physical LUND samples.
# Usage: edit the settings below locally, commit/push, then source run.csh --workflow submit on ifarm.
# Workflow: select sample -> export settings -> load GEMC -> validate -> reset outputs -> sbatch array.
# Inputs: existing OUTPATH/lundfiles/PREFIX_INDEX.txt, GCARD, YAML and the login shell's module command.
# Outputs: Slurm jobs writing OUTPATH/mchipo and OUTPATH/reconhipo; uniform also recreates rootfiles.
# Failure: checked failures jump to the return block; sourcing never exits the user's shell.
# WARNING: each submission replaces the selected sample's simulation output directories. LUND is preserved.
# All paths must be absolute and contain only letters, numbers, /, _, -, and . (protected payload contract).
# JOB_NEVENTS is an event limit shared by the array, not a claim about each input file's length.

# Editable settings -----------------------------------------------------------

# region Settings
# Select one or several named samples. Add another case in Sample settings for another run.
set samples = ( uniform-example )
# set samples = ( physical-example )
# set samples = ( uniform-example physical-example )
setenv CLEAR_FAR_OUT false
setenv CUSTOM_GEMC_VERSION true
setenv GEMC_VERSION 5.14
setenv CLAS12TAGS_DIR /lustre24/expphy/volatile/clas12/asportes/Ar40_imp_GEMC/alons126-clas12Tags
set farm_out = /u/scifarm/farm_out/asportes
# endregion Settings

# Shell environment and printing ---------------------------------------------

# region Shell support
set CLAS12_SAMPLE_STATUS = 1
set submission_outputs = ()
set farm_cleared = 0
setenv RUNNING_DIR "$cwd"
if (! -f "$RUNNING_DIR/src/slurm-submission/external/submit_GEMC_sample.sh") then
    echo "Error: source the submission script from the CLAS12-sample-generator checkout."
    goto submission_finish
endif
setenv SUBMIT_SCRIPT_FILE "$RUNNING_DIR/src/slurm-submission/external/submit_GEMC_sample.sh"
setenv SYSTEM_COLOR "`printf '\033[35m'`"
setenv COLOR_START "`printf '\033[33m'`"
setenv COLOR_END "`printf '\033[0m'`"
setenv COLOR_ERROR_START "`printf '\033[31m'`"
setenv COLOR_GOOD_START "`printf '\033[32m'`"
setenv COLOR_WARNING_START "`printf '\033[36m'`"
# The check aliases consume check_name/check_path/check_color, print the legacy messages,
# and return through the script's failure label before any later stage can run.
alias submission_dir 'echo "${check_color}--> Checking if ${COLOR_END}${check_name}${check_color} is a directory...${COLOR_END}"; test -d "$check_path"; if ($status != 0) goto submission_missing_dir; printf "${check_color}-->${COLOR_END} %s\n\n" "${COLOR_GOOD_START}${check_name} exists.${COLOR_END}"'
alias submission_file 'echo "${check_color}--> Checking if ${COLOR_END}${check_name}${check_color} is a file...${COLOR_END}"; test -f "$check_path"; if ($status != 0) goto submission_missing_file; printf "${check_color}-->${COLOR_END} %s\n\n" "${COLOR_GOOD_START}${check_name} exists.${COLOR_END}"'
alias submission_section 'echo ""; echo "${COLOR_START}=======================================================================${COLOR_END}"; printf "%s\n" "${COLOR_START}${section}${COLOR_END}"; echo "${COLOR_START}=======================================================================${COLOR_END}"; echo ""'
# endregion Shell support

foreach sample ($samples:q)
    # Sample settings ---------------------------------------------------------

    # region Sample settings
    # These are explicit inputs, not metadata inferred from directory names. Copy a case for another sample.
    # NUM_OF_JOBS selects files 1..N. JOB_NEVENTS should cover the largest selected LUND file.
    switch ("$sample")
    case uniform-example:
        set source = uniform
        setenv NUM_OF_JOBS 5000
        setenv JOB_NEVENTS 25000
        setenv TEMP_BEAM_E 2070MeV
        setenv TEMP_OUTPATH_PARTICLE enFD
        setenv TARGET_VARIATION rgm_fall2021_Ar
        setenv SAMPLE_TARGET_NUCLEUS Ar40
        setenv SAMPLE_GENERATOR uniform
        setenv GENERATOR_TUNE none
        setenv Q2_CUT none
        setenv OUTPATH_BASE /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/Uniform_samples
        setenv OUTPATH ${OUTPATH_BASE}/Uniform_sample_${TEMP_OUTPATH_PARTICLE}_${TEMP_BEAM_E}
        setenv SAMPLE_FILE_PREFIX Uniform_sample_${TEMP_OUTPATH_PARTICLE}_${TEMP_BEAM_E}
        setenv SLURM_JOB_NAME Uniform_${TEMP_OUTPATH_PARTICLE}_sample_${TEMP_BEAM_E}
        breaksw
    case physical-example:
        set source = physical
        setenv NUM_OF_JOBS 5000
        setenv JOB_NEVENTS 10000
        setenv TEMP_BEAM_E 2070MeV
        setenv TEMP_OUTPATH_PARTICLE none
        setenv TARGET_VARIATION rgm_fall2021_C_S
        setenv SAMPLE_TARGET_NUCLEUS C12
        setenv SAMPLE_GENERATOR genie
        setenv GENERATOR_TUNE GEM21_11a_00_000
        setenv Q2_CUT Q2_0_02
        # FC is a legacy naming label only; submission does not apply a fiducial cut.
        setenv FC_STATUS_ENABLED 0
        setenv FC_STATUS ""
        setenv OUTPATH_BASE /lustre24/expphy/volatile/clas12/asportes/2N_Analysis_Reco_Samples/GENIE_Reco_Samples
        # Set this to the exact completed run directory printed by create-lund.
        setenv OUTPATH ${OUTPATH_BASE}/rgm_fall2021_C_S__genie-none__GEM21_11a_00_000__Q2_0_02__2070MeV_GEMC-5.14
        setenv SAMPLE_FILE_PREFIX C12_genie_2070MeV
        setenv SLURM_JOB_NAME ${SAMPLE_TARGET_NUCLEUS}_${GENERATOR_TUNE}_${TEMP_BEAM_E}_${Q2_CUT}${FC_STATUS}_GEMC${GEMC_VERSION}
        breaksw
    default:
        echo "Error: no Sample settings case for '$sample'."
        goto submission_finish
    endsw
    # Explicit detector selection; change these here if using another reviewed card or YAML.
    switch ("$TEMP_BEAM_E")
    case 2070MeV:
        setenv TEMP_BEAM_E_ROUNDED 2GeV
        setenv TORUS_FIELD 0.5
        set yaml_name = rgm_fall2021-cv.yaml
        breaksw
    case 4029MeV:
        setenv TEMP_BEAM_E_ROUNDED 4GeV
        setenv TORUS_FIELD -1.0
        set yaml_name = rgm_fall2021-ai_4Gev.yaml
        breaksw
    case 5986MeV:
        setenv TEMP_BEAM_E_ROUNDED 6GeV
        setenv TORUS_FIELD -1.0
        set yaml_name = rgm_fall2021-ai_6Gev.yaml
        breaksw
    default:
        echo "Error: unknown beam energy: $TEMP_BEAM_E"
        goto submission_finish
    endsw
    setenv REQUIREMENTS_PATH ${RUNNING_DIR}/config/detector/Generation_files_${TEMP_BEAM_E_ROUNDED}/${GEMC_VERSION}
    setenv GCARD_FILE ${REQUIREMENTS_PATH}/${TARGET_VARIATION}_${TEMP_BEAM_E_ROUNDED}.gcard
    setenv YAML_FILE ${REQUIREMENTS_PATH}/${yaml_name}
    # endregion Sample settings

    # Setup report and modules ------------------------------------------------

    # region Setup
    if ("$source" != "uniform" && "$source" != "physical") then
        echo "Error: source must be uniform or physical."
        goto submission_finish
    endif
    if ("$CLEAR_FAR_OUT" != "true" && "$CLEAR_FAR_OUT" != "false") goto submission_bad_settings
    if ("$CUSTOM_GEMC_VERSION" != "true" && "$CUSTOM_GEMC_VERSION" != "false") goto submission_bad_settings
    printf '%s\n' "$NUM_OF_JOBS" "$JOB_NEVENTS" | awk '$0 !~ /^[1-9][0-9]*$/ {exit 1}'
    if ($status != 0) goto submission_bad_settings
    # The inherited Slurm export policy must not suppress the configured software environment.
    setenv SLURM_EXPORT_ENV ALL
    setenv SBATCH_EXPORT ALL
    setenv GENIE_TUNE "$GENERATOR_TUNE"
    echo "${COLOR_START}RUNNING_DIR::${COLOR_END} ${RUNNING_DIR}"
    echo ""
    echo
    echo ""
    echo "${SYSTEM_COLOR}///////////////////////////////////////////////////////////////////////${COLOR_END}"
    if ("$source" == "uniform") then
        printf "%s%s%s\n" "${SYSTEM_COLOR}//${COLOR_END}        Setting and submitting uniform sample generation jobs      ${SYSTEM_COLOR}//${COLOR_END}"
    else
        printf "%s%s%s\n" "${SYSTEM_COLOR}//${COLOR_END}        Setting and submitting GENIE sample generation jobs        ${SYSTEM_COLOR}//${COLOR_END}"
    endif
    echo "${SYSTEM_COLOR}///////////////////////////////////////////////////////////////////////${COLOR_END}"
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
    set check_color = "$COLOR_START"
    if ("$source" == "physical") then
        echo "${COLOR_START}OUTPATH_BASE: ${COLOR_END}${OUTPATH_BASE}"
        set check_name = OUTPATH_BASE
        set check_path = "$OUTPATH_BASE"
        submission_dir
    endif
    echo "${COLOR_START}CLAS12TAGS_DIR:${COLOR_END} ${CLAS12TAGS_DIR}"
    set check_name = CLAS12TAGS_DIR
    set check_path = "$CLAS12TAGS_DIR"
    submission_dir
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
        if (! $?GEMC_DATA_DIR) then
            echo "Error: GEMC_DATA_DIR was not set by the loaded environment."
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
    if ("$source" == "uniform") then
        set section = '= Looping over particle types                                         ='
    else
        set section = '= Looping over samples                                         ='
    endif
    submission_section
    # endregion Setup

    # Sample report -----------------------------------------------------------

    # region Sample report
    echo
    if ("$source" == "uniform") then
        echo "${COLOR_START}Processing particle type ${COLOR_END}${TEMP_OUTPATH_PARTICLE}${COLOR_START} at beam energy ${COLOR_END}${TEMP_BEAM_E}"
    else
        echo "${COLOR_START}Processing GENIE sample for ${COLOR_END}${SAMPLE_TARGET_NUCLEUS}${COLOR_START} (${COLOR_END}${GENERATOR_TUNE}${COLOR_START}) at beam energy ${COLOR_END}${TEMP_BEAM_E}"
    endif
    echo "${COLOR_START}-----------------------------------------------------------------------${COLOR_END}"
    echo
    if ("$source" == "uniform") then
        echo "${COLOR_START}TEMP_BEAM_E:${COLOR_END} ${TEMP_BEAM_E}"
        echo
        echo "${COLOR_START}TEMP_OUTPATH_PARTICLE:${COLOR_END} ${TEMP_OUTPATH_PARTICLE}"
        echo
        # FD/CD and charged-pion extensions share the corresponding legacy channel's color.
        set color_channel = "$TEMP_OUTPATH_PARTICLE"
        switch ("$color_channel")
        case enFD:
        case enCD:
            set color_channel = en
            breaksw
        case epFD:
        case epCD:
        case epipFD:
        case epipCD:
        case epimFD:
        case epimCD:
            set color_channel = ep
            breaksw
        case electron-tester:
            set color_channel = 1e
            breaksw
        endsw
        switch ("${TEMP_BEAM_E}_${color_channel}")
        case 2070MeV_1e:
            set color_code = 34
            breaksw
        case 2070MeV_en:
            set color_code = 36
            breaksw
        case 2070MeV_ep:
            set color_code = 95
            breaksw
        case 4029MeV_1e:
            set color_code = 90
            breaksw
        case 4029MeV_ep:
            set color_code = 91
            breaksw
        case 4029MeV_en:
            set color_code = 92
            breaksw
        case 5986MeV_1e:
            set color_code = 93
            breaksw
        case 5986MeV_ep:
            set color_code = 94
            breaksw
        case 5986MeV_en:
            set color_code = 96
            breaksw
        default:
            goto submission_bad_settings
        endsw
        setenv PRINT_OUT_COLOR "`printf '\033[%sm' $color_code`"
        echo "${PRINT_OUT_COLOR}OUTPATH_BASE: ${COLOR_END}${OUTPATH_BASE}"
        set check_color = "$PRINT_OUT_COLOR"
        set check_name = OUTPATH_BASE
        set check_path = "$OUTPATH_BASE"
        submission_dir
        echo "${PRINT_OUT_COLOR}TEMP_OUTPATH_PARTICLE:${COLOR_END} ${TEMP_OUTPATH_PARTICLE}"
        echo
        echo "${PRINT_OUT_COLOR}Setting environment variables based on TEMP_BEAM_E and particle type ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
        echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
        echo
    else
        echo "${COLOR_START}TARGET_VARIATION:${COLOR_END} ${TARGET_VARIATION}"
        echo
        switch ("${TEMP_BEAM_E}_${FC_STATUS_ENABLED}")
        case 2070MeV_0:
            setenv PRINT_OUT_COLOR '\033[31m'
            breaksw
        case 2070MeV_1:
            setenv PRINT_OUT_COLOR '\033[92m'
            breaksw
        case 4029MeV_0:
            setenv PRINT_OUT_COLOR '\033[33m'
            breaksw
        case 4029MeV_1:
            setenv PRINT_OUT_COLOR '\033[94m'
            breaksw
        case 5986MeV_0:
            setenv PRINT_OUT_COLOR '\033[35m'
            breaksw
        case 5986MeV_1:
            setenv PRINT_OUT_COLOR '\033[96m'
            breaksw
        default:
            goto submission_bad_settings
        endsw
    endif
    echo "${PRINT_OUT_COLOR}TEMP_BEAM_E_ROUNDED:${COLOR_END} ${TEMP_BEAM_E_ROUNDED}"
    echo
    if ("$source" == "uniform") then
        echo "${PRINT_OUT_COLOR}Setting paths based on particle type ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
        echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
        echo
    else
        echo "${PRINT_OUT_COLOR}//////////////////////////////////////////////////////////////////////${COLOR_END}"
        echo "${PRINT_OUT_COLOR}// Setting GENIE slurm job submission                               //${COLOR_END}"
        echo "${PRINT_OUT_COLOR}//////////////////////////////////////////////////////////////////////${COLOR_END}"
        echo
        echo "${PRINT_OUT_COLOR}- Sample parameters ---------------------------------------------------${COLOR_END}"
        echo ""
        echo "${PRINT_OUT_COLOR}SAMPLE_TARGET_NUCLEUS:${COLOR_END} ${SAMPLE_TARGET_NUCLEUS}"
        echo ""
        echo "${PRINT_OUT_COLOR}GENIE_TUNE:${COLOR_END} ${GENERATOR_TUNE}"
        echo ""
        echo "${PRINT_OUT_COLOR}Q2_CUT:${COLOR_END} ${Q2_CUT}"
        echo ""
        echo "${PRINT_OUT_COLOR}TEMP_BEAM_E:${COLOR_END} ${TEMP_BEAM_E}"
        echo ""
        echo "${PRINT_OUT_COLOR}FC_STATUS:${COLOR_END} ${FC_STATUS}"
        echo ""
        echo "${PRINT_OUT_COLOR}FC_STATUS_ENABLED:${COLOR_END} ${FC_STATUS_ENABLED}"
        echo ""
    endif
    echo "${PRINT_OUT_COLOR}OUTPATH:${COLOR_END} ${OUTPATH}"
    if ("$source" == "physical") echo ""
    set check_color = "$PRINT_OUT_COLOR"
    if ("$source" == "physical") set check_color = "$COLOR_START"
    # Resolve deletion targets physically and reject unsafe or aliased paths before touching outputs.
    foreach path_value ("$OUTPATH" "$GCARD_FILE" "$YAML_FILE" "$SUBMIT_SCRIPT_FILE" "$farm_out")
        printf '%s\n' "$path_value" | env LC_ALL=C awk '$0 !~ /^\/[A-Za-z0-9_.\/-]+$/ || $0 ~ /(^|\/)\.\.(\/|$)/ {exit 1}'
        if ($status != 0) goto submission_bad_settings
    end
    printf '%s\n' "$SAMPLE_FILE_PREFIX" | env LC_ALL=C awk '$0 !~ /^[A-Za-z0-9_-][A-Za-z0-9_.-]*$/ {exit 1}'
    if ($status != 0) goto submission_bad_settings
    if (! -d "$OUTPATH") then
        if ("$source" == "physical") then
            echo "${COLOR_START}--> Checking if ${COLOR_END}OUTPATH${COLOR_START} is a directory...${COLOR_END}"
            printf "%s\n" "${COLOR_START}-->${COLOR_END} ${COLOR_WARNING_START}Warning:${COLOR_END} the following directory does not exist: ${OUTPATH}"
            printf "%s\n" "${COLOR_START}-->${COLOR_END} ${COLOR_WARNING_START}Creating OUTPATH.${COLOR_END}"
            mkdir -p "$OUTPATH"
            if ($status != 0) goto submission_finish
            echo "${COLOR_START}----> Checking if ${COLOR_END}OUTPATH${COLOR_START} is a directory...${COLOR_END}"
            printf "%s\n\n" "${COLOR_START}---->${COLOR_END} ${COLOR_GOOD_START}OUTPATH was created successfully.${COLOR_END}"
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
    set resolved_out = `cd "$OUTPATH" && pwd -P`
    if ($status != 0 || "$resolved_out" == "" || "$resolved_out" == "/" || "$resolved_out" == "$HOME" || "$resolved_out" == "$RUNNING_DIR" || "$RUNNING_DIR" =~ "$resolved_out"/*) goto submission_bad_settings
    foreach previous_output ($submission_outputs:q)
        if ("$resolved_out" == "$previous_output") then
            echo "Error: samples in one invocation must use distinct OUTPATH directories."
            goto submission_finish
        endif
    end
    set submission_outputs = ($submission_outputs:q "$resolved_out")
    setenv OUTPATH "$resolved_out"
    if ("$source" == "physical") then
        echo "${PRINT_OUT_COLOR}- Job parameters ------------------------------------------------------${COLOR_END}"
        echo ""
        echo "${PRINT_OUT_COLOR}TORUS_FIELD:${COLOR_END} ${TORUS_FIELD}"
        echo ""
        set check_name = RUNNING_DIR
        set check_path = "$RUNNING_DIR"
        submission_dir
    endif
    echo "${PRINT_OUT_COLOR}REQUIREMENTS_PATH:${COLOR_END} ${REQUIREMENTS_PATH}"
    echo
    set check_color = "$PRINT_OUT_COLOR"
    set check_name = REQUIREMENTS_PATH
    set check_path = "$REQUIREMENTS_PATH"
    submission_dir
    if ("$source" == "uniform") then
        echo "${PRINT_OUT_COLOR}Setting GCARD_FILE and YAML_FILE files based on TEMP_BEAM_E and TARGET_VARIATION ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
    else
        echo "${PRINT_OUT_COLOR}Setting GCARD_FILE and YAML_FILE files based on TEMP_BEAM_E and TARGET_VARIATION${COLOR_END}"
    endif
    echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
    echo
    foreach check_name (GCARD_FILE YAML_FILE)
        if ("$check_name" == "GCARD_FILE") set check_path = "$GCARD_FILE"
        if ("$check_name" == "YAML_FILE") set check_path = "$YAML_FILE"
        echo "${PRINT_OUT_COLOR}${check_name}:${COLOR_END} ${check_path}"
        submission_file
    end
    # endregion Sample report

    # Validate LUND and prepare outputs ---------------------------------------

    # region Output preparation
    # Validate every selected index and worker command before replacing any existing simulation output.
    @ index = 1
    while ($index <= $NUM_OF_JOBS)
        if (! -s "${OUTPATH}/lundfiles/${SAMPLE_FILE_PREFIX}_${index}.txt") then
            echo "Error: missing or empty LUND input: ${OUTPATH}/lundfiles/${SAMPLE_FILE_PREFIX}_${index}.txt"
            goto submission_finish
        endif
        @ index ++
    end
    foreach executable (sbatch gemc recon-util)
        which "$executable" >& /dev/null
        if ($status != 0) then
            echo "Error: $executable is unavailable in the loaded environment."
            goto submission_finish
        endif
    end
    if ("$source" == "uniform") then
        echo "${PRINT_OUT_COLOR}Setting output directory structure ${TEMP_OUTPATH_PARTICLE}${COLOR_END}"
        set output_dirs = (mchipo reconhipo rootfiles)
    else
        echo "${PRINT_OUT_COLOR}Setting output directory structure${COLOR_END}"
        set output_dirs = (mchipo reconhipo)
    endif
    echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
    echo
    foreach directory ($output_dirs)
        if (-l "$OUTPATH/$directory") goto submission_bad_settings
    end
    echo "${PRINT_OUT_COLOR}Removing old directory structure for MC simulation here...\033[0m"
    foreach directory ($output_dirs)
        rm -rf -- "$OUTPATH/$directory"
        if ($status != 0) goto submission_finish
    end
    echo
    echo "${PRINT_OUT_COLOR}Setting up directory structure for MC simulation here...\033[0m"
    foreach directory ($output_dirs)
        mkdir "$OUTPATH/$directory"
        if ($status != 0) goto submission_finish
    end
    echo
    echo "${PRINT_OUT_COLOR}Number of files in target directory (OUTPATH):${COLOR_END}"
    echo "${PRINT_OUT_COLOR}Number of lund files:     \t\t${COLOR_END} `ls ${OUTPATH}/lundfiles | wc -l`"
    echo "${PRINT_OUT_COLOR}Number of mchipo files:   \t\t${COLOR_END} `ls ${OUTPATH}/mchipo | wc -l`"
    echo "${PRINT_OUT_COLOR}Number of reconhipo files:\t\t${COLOR_END} `ls ${OUTPATH}/reconhipo | wc -l`"
    echo
    # endregion Output preparation

    # Slurm handoff -----------------------------------------------------------

    # region Submission
    if ("$source" == "uniform") then
        echo "${PRINT_OUT_COLOR}Submitting sbatch job for BeamE = ${COLOR_END}${TEMP_BEAM_E}"
        echo "${PRINT_OUT_COLOR}-----------------------------------------------------------------------${COLOR_END}"
        echo
    else
        echo "${PRINT_OUT_COLOR}- Submitting jobs ------------------------------------------------------${COLOR_END}"
        echo ""
        echo "${PRINT_OUT_COLOR}Submitting GENIE sbatch job...${COLOR_END}"
    endif
    echo "${PRINT_OUT_COLOR}SLURM_JOB_NAME:${COLOR_END} ${SLURM_JOB_NAME}"
    echo ""
    setenv ARRAY 1-${NUM_OF_JOBS}
    echo "${PRINT_OUT_COLOR}ARRAY:${COLOR_END} ${ARRAY}"
    echo ""
    echo "${PRINT_OUT_COLOR}SUBMIT_SCRIPT_FILE:${COLOR_END} ${SUBMIT_SCRIPT_FILE}"
    set check_name = SUBMIT_SCRIPT_FILE
    set check_path = "$SUBMIT_SCRIPT_FILE"
    submission_file
    echo "${PRINT_OUT_COLOR}Submitted job with command:${COLOR_END}"
    echo "${PRINT_OUT_COLOR}sbatch --job-name=${COLOR_END}${SLURM_JOB_NAME}${PRINT_OUT_COLOR} --array=${COLOR_END}${ARRAY} ${SUBMIT_SCRIPT_FILE}"
    sbatch --job-name="$SLURM_JOB_NAME" --array="$ARRAY" "$SUBMIT_SCRIPT_FILE"
    if ($status != 0) goto submission_sbatch_failed
    echo
    echo
    # endregion Submission
end
set CLAS12_SAMPLE_STATUS = 0
goto submission_finish

# Sourced-shell failure and return --------------------------------------------

# region Return
submission_missing_dir:
printf "%s\n" "${check_color}-->${COLOR_END} ${COLOR_ERROR_START}Error:${COLOR_END} the following directory does not exist: ${check_path}"
goto submission_finish
submission_missing_file:
printf "%s\n" "${check_color}-->${COLOR_END} ${COLOR_ERROR_START}Error:${COLOR_END} the following file does not exist: ${check_path}"
goto submission_finish
submission_bad_settings:
echo "Error: invalid setting or unsafe path; check the editable submission settings."
goto submission_finish
submission_module_failed:
echo "Error: GEMC module setup failed; no subsequent sample was submitted."
goto submission_finish
submission_sbatch_failed:
echo "Error: sbatch failed; no subsequent sample was submitted."
submission_finish:
unalias submission_dir submission_file submission_section
/bin/sh -c "exit $CLAS12_SAMPLE_STATUS"
# endregion Return
