#
# Created by Alon Sportes on 14/09/2026.
#

#!/bin/tcsh

# print_success.csh -----------------------------------------------------------
# Description:
#   Presentation-only completion banner for a successfully finished checkout workflow.
# 
# Purpose:
#   Mark the end of long ifarm build, generation, or submission logs after all required commands
#   have already returned success.
# 
# Workflow:
#   1. Load the shared terminal-color palette from the checkout.
#   2. Select the completion color and print the static Unicode artwork.
#   3. Reset terminal formatting and terminate the banner with a newline.
# 
# Usage:
#   Invoke from the repository root through workflow.py's banner helper.
# 
# Inputs:
#   The sourced color helper exports COLOR_COMPLETION and COLOR_END.
# 
# Outputs:
#   Writes only to standard output and does not modify workflow state or generated artifacts.
# 
# Failure behavior:
#   The caller treats printer failures as presentation failures; they do not replace the completed
#   workflow's result.

# Color initialization --------------------------------------------------------

# region Color initialization
# Reload the palette so direct invocation and calls from different shells render consistently.
source ./src/launcher/environment/set_colors.csh
# endregion

# Banner rendering ------------------------------------------------------------

# region Banner rendering
# Begin green completion output without adding an extra line before the artwork.
printf "$COLOR_COMPLETION"

# Keep the wide artwork readable as a heredoc. The filter supports the shared convention in which
# @ represents a literal dollar sign without triggering shell expansion in banner text.
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

# Restore default terminal formatting, then finish on a clean line for the caller's next prompt.
printf "$COLOR_END"
echo ""
# endregion
