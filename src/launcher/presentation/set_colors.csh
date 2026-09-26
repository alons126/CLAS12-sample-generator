#!/bin/tcsh

#
# Created by Alon Sportes on 15/09/2026.
#

# Terminal color environment --------------------------------------------------

# Description:
#   Define the terminal colors shared by maintained shell, Python, and C++ output.

# region Terminal color environment
# Purpose:
#   Keep terminal color values in one shell file.
# 
# Workflow:
#   Remove old local and exported values -> export the current palette -> let child programs inherit it.
#
# Inputs:
#   None. Re-sourcing always replaces stale values.
# 
# Outputs:
#   Exported `*_COLOR` variables remain available to run.csh, shared printer helpers, and child processes.
#
# Usage:
#   Source this file before printing with a `*_COLOR` variable.
# 
# Notes:
#   SYSTEM_COLOR is the normal heading color. RESET_COLOR stops color from leaking into later output.
#   The file prints nothing.

unset ERROR_COLOR COMPLETION_COLOR SYSTEM_COLOR INFO_COLOR WARNING_COLOR RESET_COLOR
unsetenv ERROR_COLOR COMPLETION_COLOR SYSTEM_COLOR INFO_COLOR WARNING_COLOR RESET_COLOR

setenv ERROR_COLOR      '\033[31m'  # Red
setenv COMPLETION_COLOR "\033[32m"  # Green
setenv SYSTEM_COLOR     '\033[33m'  # Yellow
setenv INFO_COLOR       '\033[35m'  # Magenta
setenv WARNING_COLOR    '\033[36m'  # Cyan
setenv RESET_COLOR      '\033[0m'   # Reset color
# endregion
