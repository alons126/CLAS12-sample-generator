#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# print_success.csh -----------------------------------------------------------
# Description:
#   Print the banner for a completed workflow.
# 
# Purpose:
#   Clearly mark the end of a successful terminal or ifarm log.
# 
# Workflow:
#   1. Load the shared colors.
#   2. Print the artwork in the completion color.
#   3. Restore the normal terminal color.
# 
# Usage:
#   A workflow calls this script once after every requested sample or stage succeeds.
# 
# Inputs:
#   The sourced color helper exports COMPLETION_COLOR and RESET_COLOR.
# 
# Outputs:
#   Prints the banner. It does not change workflow files or state.
# 
# Failure behavior:
#   A banner error does not change the completed workflow's result.

# Color initialization --------------------------------------------------------

# region Color initialization
# Load the color settings even when this script is called directly.
source ./src/launcher/presentation/set_colors.csh
# endregion

# Banner rendering ------------------------------------------------------------

# region Banner rendering
# Start the completion color without adding a line.
printf "$COMPLETION_COLOR"

# Keep the artwork literal and use `@` as a safe placeholder for a dollar sign.
cat << EOF | sed 's/@/\$/g'
####################################################################################################
####################################################################################################

          ███████╗██╗███╗   ██╗██╗███████╗██╗  ██╗███████╗██████╗
          ██╔════╝██║████╗  ██║██║██╔════╝██║  ██║██╔════╝██╔══██╗
          █████╗  ██║██╔██╗ ██║██║███████╗███████║█████╗  ██║  ██║
          ██╔══╝  ██║██║╚██╗██║██║╚════██║██╔══██║██╔══╝  ██║  ██║
          ██║     ██║██║ ╚████║██║███████║██║  ██║███████╗██████╔╝
          ╚═╝     ╚═╝╚═╝  ╚═══╝╚═╝╚══════╝╚═╝  ╚═╝╚══════╝╚═════╝
███████╗██╗   ██╗ ██████╗ ██████╗███████╗███████╗███████╗███████╗██╗   ██╗██╗     ██╗  ██╗   ██╗
██╔════╝██║   ██║██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝██║   ██║██║     ██║  ╚██╗ ██╔╝
███████╗██║   ██║██║     ██║     █████╗  ███████╗███████╗█████╗  ██║   ██║██║     ██║   ╚████╔╝
╚════██║██║   ██║██║     ██║     ██╔══╝  ╚════██║╚════██║██╔══╝  ██║   ██║██║     ██║    ╚██╔╝
███████║╚██████╔╝╚██████╗╚██████╗███████╗███████║███████║██║     ╚██████╔╝███████╗███████╗██║
╚══════╝ ╚═════╝  ╚═════╝ ╚═════╝╚══════╝╚══════╝╚══════╝╚═╝      ╚═════╝ ╚══════╝╚══════╝╚═╝

####################################################################################################
####################################################################################################
EOF

# Restore the normal color and finish on a new line.
printf "$RESET_COLOR"
echo ""
# endregion
