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
#   precedence over environment variables during `$COLOR_*` expansion. It then replaces stale
#   exported values. Shell output interprets the escape sequence, while workflow.py converts the
#   literal `\033` prefix before using the inherited environment.
# 
# Outputs:
#   Exported COLOR_* variables remain available to run.csh, printer helpers, and child processes.
# 
# Notes:
#   COLOR_START is the default heading color. COLOR_END must follow colored text so later terminal
#   output does not inherit the style. Clearing both tcsh namespaces makes this file recover from a
#   contaminated interactive shell. These assignments intentionally contain no terminal output.

unset COLOR_START COLOR_ERR COLOR_COMPLETION COLOR_INFO COLOR_WARNING COLOR_END
unsetenv COLOR_START COLOR_ERR COLOR_COMPLETION COLOR_INFO COLOR_WARNING COLOR_END

setenv COLOR_START      '\033[33m'  # Yellow
setenv COLOR_ERR        '\033[31m'  # Red
setenv COLOR_COMPLETION "\033[32m"  # Green
setenv COLOR_INFO       '\033[35m'  # Magenta
setenv COLOR_WARNING    '\033[36m'  # Cyan
setenv COLOR_END        '\033[0m'   # Reset color
# endregion
