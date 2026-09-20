#
# Created by Alon Sportes on 14/09/2026.
#

#!/bin/tcsh

# print_logo.csh --------------------------------------------------------------
# Description:
#   Presentation-only CLAS12 sample-generator startup banner.
# Purpose:
#   Make the beginning of a long ifarm update/build log easy to identify without changing workflow
#   configuration, environment state, or success/failure decisions.
# Workflow:
#   1. Select blue ANSI output.
#   2. Stream the literal banner through the established @-to-$ placeholder filter.
#   3. Reset terminal formatting after the final separator.
# Inputs:
#   None. The banner is static and does not read sample or launcher settings.
# Outputs:
#   Writes only to standard output. run.csh treats this helper as optional presentation.
# Failure behavior:
#   A printer failure must not determine whether synchronization, build, or submission proceeds.

# Banner rendering ------------------------------------------------------------

# region Banner rendering
# Set the banner color directly so this standalone helper does not require set_colors.csh.
echo "\033[34m"

# The heredoc keeps the wide Unicode artwork readable in source form. The sed filter supports the
# project's banner placeholder convention, where @ may stand in for a literal dollar sign without
# inviting shell-variable expansion inside the heredoc.
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

# Restore the terminal default so commands printed after the banner are not blue.
echo "\033[0m"
# endregion
