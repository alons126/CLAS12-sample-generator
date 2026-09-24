#!/bin/tcsh

#
# Created by Alon Sportes on 15/09/2026.
#

# Terminal color environment --------------------------------------------------

# region Terminal color environment
# Purpose:
#   Publish one shared ANSI color palette for sourced tcsh helpers and the Python workflow driver.
# 
# Workflow:
#   Re-sourcing this file first removes same-named tcsh local variables, which otherwise take
#   precedence over environment variables during `*_COLOR` expansion. It then replaces stale
#   exported values. Shell output interprets the escape sequence, while workflow.py and the C++
#   environment adapter convert the literal `\033` prefix before using the inherited environment.
# 
# Outputs:
#   Exported `*_COLOR` variables remain available to run.csh, printer helpers, and child processes.
# 
# Notes:
#   SYSTEM_COLOR is the default heading color. RESET_COLOR must follow colored text so later terminal
#   output does not inherit the style. Clearing both tcsh namespaces makes this file recover from a
#   contaminated interactive shell. These assignments intentionally contain no terminal output.

unset ERROR_COLOR COMPLETION_COLOR SYSTEM_COLOR INFO_COLOR WARNING_COLOR RESET_COLOR
unsetenv ERROR_COLOR COMPLETION_COLOR SYSTEM_COLOR INFO_COLOR WARNING_COLOR RESET_COLOR

setenv ERROR_COLOR      '\033[31m'  # Red
setenv COMPLETION_COLOR "\033[32m"  # Green
setenv SYSTEM_COLOR     '\033[33m'  # Yellow
setenv INFO_COLOR       '\033[35m'  # Magenta
setenv WARNING_COLOR    '\033[36m'  # Cyan
setenv RESET_COLOR      '\033[0m'   # Reset color
# endregion
