#!/bin/csh

# Setup
# ---------------------------------------------------------------------------

# Colors (override by exporting COLOR_START / COLOR_END before running)
# Use real ESC bytes so output is colored (printf interprets these correctly)
unsetenv SYSTEM_COLOR
setenv SYSTEM_COLOR "`printf '\033[35m'`"

unsetenv COLOR_START
setenv COLOR_START "`printf '\033[33m'`"

unsetenv COLOR_END
setenv COLOR_END "`printf '\033[0m'`"

unsetenv COLOR_ERROR_START
setenv COLOR_ERROR_START "`printf '\033[31m'`"

unsetenv COLOR_GOOD_START
setenv COLOR_GOOD_START "`printf '\033[32m'`"

unsetenv COLOR_WARNING_START
setenv COLOR_WARNING_START "`printf '\033[36m'`"

# Setting RUNNING_DIR
unsetenv RUNNING_DIR
setenv RUNNING_DIR `pwd`
echo "${COLOR_START}RUNNING_DIR::${COLOR_END} ${RUNNING_DIR}"
echo ""

# Functions (csh/tcsh-compatible aliases)
# ---------------------------------------------------------------------------
# Usage:
#   banner "Some text"
#   Check_if_dir_exist "/path/to/dir"
#   Check_if_file_exist "/path/to/file"

alias banner 'echo ""; \
  echo "${COLOR_START}=======================================================================${COLOR_END}"; \
  printf "%s%s%s\n" "${COLOR_START}= " "\!*" " =${COLOR_END}"; \
  echo "${COLOR_START}=======================================================================${COLOR_END}"; \
  echo ""'

alias Check_if_dir_exist 'if ( ! -d "\!*" ) then \
  echo "${COLOR_ERROR_START}Error:${COLOR_END} the following directory does not exist: \!*"; \
  exit 1; \
endif'

alias Check_if_file_exist 'if ( ! -f "\!*" ) then \
  echo "${COLOR_ERROR_START}Error:${COLOR_END} the following file does not exist: \!*"; \
  exit 1; \
endif'
