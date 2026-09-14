#!/bin/csh

# Setup
# ---------------------------------------------------------------------------
source ./scripts/set_env.csh
echo

source ./scripts/update_script.csh
echo

# Run setup and submission scripts:
# ---------------------------------------------------------------------------
source ./scripts/setup_and_submission_scripts/run_setup_and_submission_scripts.csh

echo "${SYSTEM_COLOR}- Operation finished --------------------------------------------------${COLOR_END}"
echo ""
