#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# print_stop.csh -----------------------------------------------------------------------------------
# Description:
#   Print the orange "operation cancelled" banner used when a maintained workflow stops early.
# 
# Purpose:
#   Clearly mark a stopped workflow in the terminal or ifarm log.
# 
# Workflow:
#   1. Select the orange text color.
#   2. Print the banner and replace `@` placeholders with dollar signs.
#   3. Restore the normal terminal color.
# 
# Inputs:
#   None. The caller prints the diagnostic message before invoking this presentation helper.
# 
# Outputs:
#   Prints the colored banner. It creates no files and keeps no shell changes.
# 
# Usage:
#   Run this script after an error. The caller must keep and return the original failure status.
# 
# Failure behavior:
#   This helper does not handle the error or choose an exit status.

# Failure-banner rendering -------------------------------------------------------------------------

# region Failure-banner rendering
# Orange distinguishes a stopped/cancelled operation from the normal success banner.
echo "\033[38;5;208m"

# Keep the artwork literal and use `@` as a safe placeholder for a dollar sign.
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

# Restore the normal color for later messages.
echo "\033[0m"
echo ""
# endregion Failure-banner rendering
