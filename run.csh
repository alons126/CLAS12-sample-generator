
#!/bin/tcsh

# -------------------------------------------------------------------------------------------------
# run.csh
# -------------------------------------------------------------------------------------------------
# Main entry script for the 2N-Analyzer workflow.
# This script performs three possible operations in sequence:
#   1) Update the repository (git pull and cleanup)
#   2) Configure and build the C++ analysis code with CMake
#   3) Run the analysis executable (TwoNAnalyzerReco)
#
# The behavior can be controlled with command-line flags:
#   --git-pull <true|false>   enable or disable repository update
#   --build    <true|false>   enable or disable CMake configure/build
#   --run      <true|false>   enable or disable running the executable
#
# Any other arguments are forwarded directly to the C++ executable.
# -------------------------------------------------------------------------------------------------

# Usage:
#   ./run.csh
#     Pulls latest changes, configures/builds, then runs TwoNAnalyzerReco.
#
#   ./run.csh --git-pull false
#     Skips git pull, but still builds and runs.
#
#   ./run.csh --build false
#     Skips configure/build, but still runs the existing executable.
#
#   ./run.csh --run false
#     Pulls and builds, but does not run the executable.
#
#   ./run.csh --git-pull false --build false --run true --sample-chain config/samples/RecoSampleChain_Data.yaml
#     Uses existing build and forwards the remaining arguments to TwoNAnalyzerReco.
#
# Typical SSH workflow:
#   ssh <user>@<host>
#   cd /path/to/2N-Analyzer-e4nu
#   ./run.csh
#
# SSH-friendly helper:
# 1) Optionally update local repo
# 2) Optionally configure + build with CMake
# 3) Optionally run the reco executable

# Determine how this script was invoked.
# If the script is executed normally (./run.csh), $0 contains the script path.
# If the script is sourced (source run.csh), $0 may contain the shell name
# (for example "tcsh" or "-tcsh").  We detect this and treat the current
# working directory as the script directory in that case.
# 
# Resolve script directory and switch to it so all relative paths work.
# When sourced (`source run.csh`), $0 may be the shell name or shell path
# (for example "-tcsh" or "/bin/tcsh"), so we fall back to current working
# directory in those cases.
set invocation = "$0"
set invocation_base = `basename -- "$invocation"`

# Flag indicating whether the script was sourced instead of executed.
# This matters because sourced scripts must use `return` instead of `exit`
# when aborting.
set IS_SOURCED = 0

# Flag that records whether execution was interrupted with Ctrl-C.
# This prevents the success banner from printing when the run was aborted.
set INTERRUPTED = 0
if ("$invocation" =~ "-*" || "$invocation_base" == "tcsh" || "$invocation_base" == "csh") then
    set script_dir = "$cwd"
    set IS_SOURCED = 1
else
    set script_dir = `dirname -- "$invocation"`
    if ($status != 0) set script_dir = "$cwd"
endif
cd "$script_dir"
set script_dir = `pwd`

# Register a Ctrl-C (SIGINT) handler. If the user presses Ctrl-C anywhere
# during the workflow, execution jumps to the RUN_INTERRUPTED label below.
onintr RUN_INTERRUPTED

# Default workflow control flags.
# These can be overridden using command-line arguments parsed below.
set DO_GIT_PULL = 1
set DO_BUILD = 1
set DO_RUN = 1

# Arguments that are not recognized as script flags are collected here and
# later forwarded to the C++ executable.
set forwarded_args = ()

# Parse command-line arguments.
# Script-level flags are interpreted here while everything else is stored
# and passed through to the analysis executable.
@ i = 1
while ($i <= $#argv)
    set arg = "$argv[$i]"

    if ("$arg" == "--git-pull") then
        @ i++
        if ($i > $#argv) then
            echo "${COLOR_ERR}Missing value for --git-pull. Use true or false.${COLOR_END}"
            exit 1
        endif
        set value = "$argv[$i]"
        switch ("$value")
            case true:
            case TRUE:
            case 1:
            case yes:
            case YES:
            case on:
            case ON:
                set DO_GIT_PULL = 1
                breaksw
            case false:
            case FALSE:
            case 0:
            case no:
            case NO:
            case off:
            case OFF:
                set DO_GIT_PULL = 0
                breaksw
            default:
                echo "${COLOR_ERR}Invalid value for --git-pull: $value. Use true or false.${COLOR_END}"
                exit 1
        endsw
    else if ("$arg" == "--build") then
        @ i++
        if ($i > $#argv) then
            echo "${COLOR_ERR}Missing value for --build. Use true or false.${COLOR_END}"
            exit 1
        endif
        set value = "$argv[$i]"
        switch ("$value")
            case true:
            case TRUE:
            case 1:
            case yes:
            case YES:
            case on:
            case ON:
                set DO_BUILD = 1
                breaksw
            case false:
            case FALSE:
            case 0:
            case no:
            case NO:
            case off:
            case OFF:
                set DO_BUILD = 0
                breaksw
            default:
                echo "${COLOR_ERR}Invalid value for --build: $value. Use true or false.${COLOR_END}"
                exit 1
        endsw
    else if ("$arg" == "--run") then
        @ i++
        if ($i > $#argv) then
            echo "${COLOR_ERR}Missing value for --run. Use true or false.${COLOR_END}"
            exit 1
        endif
        set value = "$argv[$i]"
        switch ("$value")
            case true:
            case TRUE:
            case 1:
            case yes:
            case YES:
            case on:
            case ON:
                set DO_RUN = 1
                breaksw
            case false:
            case FALSE:
            case 0:
            case no:
            case NO:
            case off:
            case OFF:
                set DO_RUN = 0
                breaksw
            default:
                echo "${COLOR_ERR}Invalid value for --run: $value. Use true or false.${COLOR_END}"
                exit 1
        endsw
    else
        set forwarded_args = ($forwarded_args "$arg")
    endif

    @ i++
end

# Terminal color escape sequences used for formatted output messages.
set COLOR_START = "\033[35m"
set COLOR_ERR = "\033[31m"
set COLOR_OK = "\033[32m"
set COLOR_END = "\033[0m"

# Print the project ASCII logo banner.
# The banner script lives under scripts/printers/.
set PRINT_LOGO_FILE = ./scripts/printers/print_logo.csh
if (-f ${PRINT_LOGO_FILE}) then
    source ${PRINT_LOGO_FILE}
else
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following file does not exist: ${PRINT_LOGO_FILE}"
    exit 1
endif

# Run optional repository update helper script.
# This typically performs git cleanup/reset and other maintenance steps.
set UPDATE_ONLY_FILE = ./update_only.sh
if (-f ${UPDATE_ONLY_FILE}) then
    source ${UPDATE_ONLY_FILE}
else
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following file does not exist: ${UPDATE_ONLY_FILE}"
    exit 1
endif

# Main build/run driver script.
# This script performs the CMake configuration, compilation, and execution
# of the TwoNAnalyzerReco binary.
set BUILD_AND_RUN_FILE = ./scripts/build_and_run.csh
if (-f ${BUILD_AND_RUN_FILE}) then
    source ${BUILD_AND_RUN_FILE}

    # Capture the exit status of the build/run script.
    # Exit code 130 indicates the process was interrupted by Ctrl-C.
    # Any other non-zero status is treated as a normal failure and
    # should show the stop banner instead of the success banner.
    set RUN_STATUS = $status
    if ($RUN_STATUS == 130) then
        set INTERRUPTED = 1
        goto RUN_ABORTED
    else if ($RUN_STATUS != 0) then
        source ./scripts/printers/print_stop.csh
        goto RUN_ABORTED
    endif
else
    printf "${COLOR_START}-->${COLOR_END} %s%s%s\n" "${COLOR_ERROR_START}Error:${COLOR_END}" " the following file does not exist: ${BUILD_AND_RUN_FILE}"
    exit 1
endif

goto RUN_FINISHED

# Interrupt handler. Triggered when the user presses Ctrl-C.
# Prints the stop banner and prevents the normal success banner
# from appearing.
RUN_INTERRUPTED:
set INTERRUPTED = 1
source ./scripts/printers/print_stop.csh

if ($IS_SOURCED) then
    goto RUN_ABORTED
else
    exit 130
endif

# Normal completion path.
# The success banner is printed only if the workflow was not interrupted.
RUN_FINISHED:
if ($INTERRUPTED == 0) then
    source ./scripts/printers/print_success.csh
endif

RUN_ABORTED:
