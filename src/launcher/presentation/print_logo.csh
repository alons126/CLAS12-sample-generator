#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# print_logo.csh --------------------------------------------------------------
# Description:
#   Print the CLAS12 sample-generator startup banner.
# 
# Purpose:
#   Make the beginning of a long ifarm log easy to find. This script does not change settings or decide
#   whether the workflow succeeds.
# 
# Workflow:
#   1. Load the shared palette and select its blue logo color.
#   2. Print the banner and replace each `@` placeholder with a dollar sign.
#   3. Reset terminal formatting after the final separator.
# 
# Inputs:
#   LOGO_COLOR and RESET_COLOR from the shared palette.
# 
# Outputs:
#   Writes only to the terminal. run.csh can continue if this optional banner fails.
#
# Usage:
#   Run from the repository root when a startup banner is wanted.
# 
# Failure behavior:
#   A printing error does not stop an update, build, or submission.

# Banner rendering ------------------------------------------------------------

# region Banner rendering
# Load the centralized palette even when this printer is called directly.
source ./src/launcher/presentation/set_colors.csh

# Start the blue logo color.
printf "$LOGO_COLOR"

# Keep the artwork readable here. Replace `@` with a literal dollar sign when printing.
cat << EOF | sed 's/@/\$/g'
####################################################################################################
####################################################################################################

 ██████╗██╗      █████╗ ███████╗ ██╗██████╗     ███████╗ █████╗ ███╗   ███╗██████╗ ██╗     ███████╗
██╔════╝██║     ██╔══██╗██╔════╝███║╚════██╗    ██╔════╝██╔══██╗████╗ ████║██╔══██╗██║     ██╔════╝
██║     ██║     ███████║███████╗╚██║ █████╔╝    ███████╗███████║██╔████╔██║██████╔╝██║     █████╗
██║     ██║     ██╔══██║╚════██║ ██║██╔═══╝     ╚════██║██╔══██║██║╚██╔╝██║██╔═══╝ ██║     ██╔══╝
╚██████╗███████╗██║  ██║███████║ ██║███████╗    ███████║██║  ██║██║ ╚═╝ ██║██║     ███████╗███████╗
 ╚═════╝╚══════╝╚═╝  ╚═╝╚══════╝ ╚═╝╚══════╝    ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝╚══════╝
           ██████╗ ███████╗███╗   ██╗███████╗██████╗  █████╗ ████████╗ ██████╗ ██████╗
           ██╔════╝ ██╔════╝████╗  ██║██╔════╝██╔══██╗██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗
           ██║  ███╗█████╗  ██╔██╗ ██║█████╗  ██████╔╝███████║   ██║   ██║   ██║██████╔╝
           ██║   ██║██╔══╝  ██║╚██╗██║██╔══╝  ██╔══██╗██╔══██║   ██║   ██║   ██║██╔══██╗
           ╚██████╔╝███████╗██║ ╚████║███████╗██║  ██║██║  ██║   ██║   ╚██████╔╝██║  ██║
           ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝

By: Alon Sportes for e4nu

####################################################################################################
####################################################################################################
EOF

# Restore the normal terminal color.
printf "$RESET_COLOR"
# endregion
