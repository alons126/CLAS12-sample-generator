#!/bin/tcsh

# print_logo.csh ----------------------------------------------------------
# Description:
#   Display the CLAS12 logo banner.
# Workflow context:
#   workflow.py invokes this presentation helper around workflow execution.
# Inputs: None.
# Outputs: Terminal text only; the driver owns the workflow exit status.

# region Banner output
printf "%s\n" "CLAS12 sample generation | uniform / GENIE / GEMC / Slurm"
