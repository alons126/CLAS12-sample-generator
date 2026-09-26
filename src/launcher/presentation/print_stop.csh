#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# print_stop.csh -----------------------------------------------------------------------------------
# Description:
#   Print the orange "operation cancelled" banner used when a project workflow stops early.
# 
# Purpose:
#   Clearly mark a stopped workflow in the terminal or ifarm log.
# 
# Workflow:
#   1. Load the shared palette and select its orange stop color.
#   2. Print the banner and replace `@` placeholders with dollar signs.
#   3. Restore the normal terminal color.
# 
# Inputs:
#   STOP_COLOR and RESET_COLOR from the shared palette.
# 
# Outputs:
#   Prints the colored banner. It creates no files and keeps no shell changes.
# 
# Usage:
#   A workflow calls this script before printing its final error. The workflow keeps the original
#   failure status.
# 
# Failure behavior:
#   This helper does not handle the error or choose an exit status.

# Failure-banner rendering -------------------------------------------------------------------------

# region Failure-banner rendering
# Load the centralized palette even when this printer is called directly.
source ./src/launcher/presentation/set_colors.csh

# Orange distinguishes a stopped/cancelled operation from the normal success banner.
printf "$STOP_COLOR"

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
printf "$RESET_COLOR"
echo ""
# endregion Failure-banner rendering
