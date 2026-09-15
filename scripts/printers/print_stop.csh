#!/bin/tcsh

# print_stop.csh ----------------------------------------------------------
# Description:
#   Display the CLAS12 stop banner.
# Workflow context:
#   workflow.py invokes this presentation helper around workflow execution.
# Inputs: None.
# Outputs: Terminal text only; the driver owns the workflow exit status.

# region Banner output
printf "%s\n" "CLAS12 workflow stopped; inspect the preceding error."
