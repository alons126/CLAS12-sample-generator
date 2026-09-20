#
# Created by Alon Sportes on 15/09/2026.
#

#!/bin/tcsh

# Terminal color environment --------------------------------------------------

# region Terminal color environment
# Purpose:
#   Publish one shared ANSI color palette for sourced tcsh helpers and the Python workflow driver.
# Workflow:
#   Re-sourcing this file replaces stale caller values; shell output interprets the escape sequence,
#   while workflow.py converts the literal `\033` prefix before using the inherited environment.
# Outputs:
#   Exported COLOR_* variables remain available to run.csh, printer helpers, and child processes.
# Notes:
#   COLOR_START is the default heading color. COLOR_END must follow colored text so later terminal
#   output does not inherit the style. These assignments intentionally contain no terminal output.
setenv COLOR_START      '\033[33m'  # Yellow
setenv COLOR_ERR        '\033[31m'  # Red
setenv COLOR_COMPLETION "\033[32m"  # Green
setenv COLOR_INFO       '\033[35m'  # Magenta
setenv COLOR_WARNING    '\033[36m'  # Cyan
setenv COLOR_END        '\033[0m'   # Reset color
# endregion
