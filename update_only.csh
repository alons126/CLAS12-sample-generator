#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# update_only.csh --------------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   C-shell entry point for refreshing the disposable ifarm checkout without building or running a workflow.
#
# Purpose:
#   Give users one command that safely updates the ifarm checkout.
#
# Workflow:
#   Run the shared updater -> show its output -> return its status to the calling C shell.
#
# Inputs:
#   The current checkout, its Git remote and branch, and the build directories that must be kept.
#
# Outputs:
#   A synchronized disposable checkout. No build, LUND creation, or submission is started.
#
# Failure:
#   A failed checkout check, cleanup, reset, pull, or submodule update returns a nonzero status. Uncommitted
#   files on the server may be removed because the ifarm checkout is only a copy used for running jobs.
#
# Usage:
#   source update_only.csh

# Checkout update --------------------------------------------------------------------------------------------------------------------------------------------------------

# region Checkout update
source ./src/launcher/checkout/code_updater.csh
# endregion
