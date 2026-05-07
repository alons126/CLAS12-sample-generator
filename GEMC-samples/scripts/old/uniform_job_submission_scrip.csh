#!/bin/csh

source uniform_setup_and_submit.csh

echo "- Submmiting 2070MeV jobs ---------------------------------------------"

uniform_setup_and_submit 2070MeV false
# uniform_setup_and_submit 2070MeV true

echo "- Submmiting 4029MeV jobs ---------------------------------------------"

uniform_setup_and_submit 4029MeV

echo "- Submmiting 5986MeV jobs ---------------------------------------------"

uniform_setup_and_submit 5986MeV

echo "- Submission finished -------------------------------------------------"
