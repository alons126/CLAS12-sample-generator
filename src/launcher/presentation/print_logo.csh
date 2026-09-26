#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# print_logo.csh --------------------------------------------------------------
# Description:
#   Presentation-only CLAS12 sample-generator startup banner.
# 
# Purpose:
#   Make the beginning of a long ifarm update/build log easy to identify without changing workflow
#   configuration, environment state, or success/failure decisions.
# 
# Workflow:
#   1. Select blue ANSI output.
#   2. Stream the literal banner through the established @-to-$ placeholder filter.
#   3. Reset terminal formatting after the final separator.
# 
# Inputs:
#   None. The banner is static and does not read sample or launcher settings.
# 
# Outputs:
#   Writes only to standard output. run.csh treats this helper as optional presentation.
#
# Usage:
#   Run from the repository root when a startup banner is wanted.
# 
# Failure behavior:
#   A printer failure must not determine whether synchronization, build, or submission proceeds.

# Banner rendering ------------------------------------------------------------

# region Banner rendering
# Start the blue logo color.
echo "\033[34m"

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
echo "\033[0m"
# endregion
