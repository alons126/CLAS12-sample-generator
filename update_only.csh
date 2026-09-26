#!/bin/tcsh

#
# Created by Alon Sportes on 14/09/2026.
#

# update_only.csh --------------------------------------------------------------------------------------------------------------------------------------------------------
# Description:
#   C-shell entry point for refreshing the disposable ifarm checkout without building or running a workflow.
#
# Purpose:
#   Expose the guarded repository synchronization owned by src/launcher/checkout/code_updater.csh as one explicit
#   operator command.
#
# Workflow:
#   Source the maintained updater -> preserve its output and final status for the calling C shell.
#
# Inputs:
#   The current checkout, its configured Git remote/branch, and the exclusions enforced by the updater.
#
# Outputs:
#   A synchronized disposable checkout. No build, LUND creation, or submission is started.
#
# Failure:
#   Repository identification, cleanup, reset, pull, or submodule failures propagate through the sourced
#   updater. Server-side uncommitted work is intentionally disposable under this operational contract.
#
# Usage:
#   source update_only.csh

# Checkout update --------------------------------------------------------------------------------------------------------------------------------------------------------

# region Checkout update
source ./src/launcher/checkout/code_updater.csh
# endregion
