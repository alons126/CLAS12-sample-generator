#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# print_stop.csh -----------------------------------------------------------------------------------
# Description:
#   Print the orange "operation cancelled" banner used when a maintained workflow stops early.
# 
# Purpose:
#   Give interactive terminals and ifarm logs a clear visual boundary after the caller reports the
#   error that caused the workflow to stop.
# 
# Workflow:
#   1. Select the banner's ANSI foreground color.
#   2. Stream the literal banner through sed so any placeholder `@` characters would be rendered as
#      dollar signs without allowing tcsh to expand variables inside the here-document.
#   3. Restore the terminal's default formatting and add a trailing blank line.
# 
# Inputs:
#   None. The caller prints the diagnostic message before invoking this presentation helper.
# 
# Outputs:
#   Writes ANSI color controls and the banner to standard output. It creates no files and changes no
#   persistent shell state.
# 
# Usage:
#   Invoke this script from the workflow error path; the calling workflow remains responsible for
#   preserving and returning the underlying nonzero exit status.
# 
# Failure behavior:
#   This helper does not interpret errors or choose an exit status. A rendering failure is local to
#   the banner and must not replace the workflow failure already captured by the caller.

# Failure-banner rendering -------------------------------------------------------------------------

# region Failure-banner rendering
# Orange distinguishes a stopped/cancelled operation from the normal success banner.
echo "\033[38;5;208m"

# The quoted delimiter keeps the artwork literal. The sed filter preserves the convention used by
# the other banner helpers, where `@` is a safe placeholder for a displayed dollar sign.
cat << \EOF | sed 's/@/\$/g'
####################################################################################################
####################################################################################################

    ██████╗ ██████╗ ███████╗██████╗  █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
    ██╔═══██╗██╔══██╗██╔════╝██╔══██╗██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
    ██║   ██║██████╔╝█████╗  ██████╔╝███████║   ██║   ██║██║   ██║██╔██╗ ██║
    ██║   ██║██╔═══╝ ██╔══╝  ██╔══██╗██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
    ╚██████╔╝██║     ███████╗██║  ██║██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
    ╚═════╝ ╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝

    ██████╗ █████╗ ███╗   ██╗ ██████╗███████╗██╗     ██╗     ███████╗██████╗ ██╗
    ██╔════╝██╔══██╗████╗  ██║██╔════╝██╔════╝██║     ██║     ██╔════╝██╔══██╗██║
    ██║     ███████║██╔██╗ ██║██║     █████╗  ██║     ██║     █████╗  ██║  ██║██║
    ██║     ██╔══██║██║╚██╗██║██║     ██╔══╝  ██║     ██║     ██╔══╝  ██║  ██║╚═╝
    ╚██████╗██║  ██║██║ ╚████║╚██████╗███████╗███████╗███████╗███████╗██████╔╝██╗
    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝╚══════╝╚══════╝╚══════╝╚══════╝╚═════╝ ╚═╝

####################################################################################################
####################################################################################################
\EOF

# Reset formatting so later terminal messages do not inherit the banner color.
echo "\033[0m"
echo ""
# endregion Failure-banner rendering
