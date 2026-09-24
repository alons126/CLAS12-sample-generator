#!/bin/tcsh

#
# Created by Alon Sportes on 20/09/2026.
#

if ( -f ./src/launcher/environment/set_colors.csh ) then
    source ./src/launcher/environment/set_colors.csh
else
    echo "\033[31mError:\033[0m the following file does not exist: ./src/launcher/environment/set_colors.csh\n"
    exit 1
endif

# Render every titled banner at 100 visible columns. Callers supply banner_title and banner_color;
# the helper calculates asymmetric padding when an odd number of spaces is required.
alias code_banner 'set banner_title_length = `printf "%s" "$banner_title" | wc -c`; @ banner_padding = 96 - $banner_title_length; @ banner_padding_left = $banner_padding / 2; @ banner_padding_right = $banner_padding - $banner_padding_left; echo "${banner_color}////////////////////////////////////////////////////////////////////////////////////////////////////${RESET_COLOR}"; printf "${banner_color}//%*s${RESET_COLOR}%s${banner_color}%*s//${RESET_COLOR}\n" $banner_padding_left "" "$banner_title" $banner_padding_right ""; echo "${banner_color}////////////////////////////////////////////////////////////////////////////////////////////////////${RESET_COLOR}"; unset banner_title banner_color banner_title_length banner_padding banner_padding_left banner_padding_right'

# Render every titled banner at 100 visible columns. Callers supply banner_title and banner_color;
# the helper calculates asymmetric padding when an odd number of spaces is required.
alias code_subbanner 'set subbanner_title_length = `printf "%s" "$subbanner_title" | wc -c`; @ subbanner_padding = 96 - $subbanner_title_length; @ subbanner_padding_left = $subbanner_padding / 2; @ subbanner_padding_right = $subbanner_padding - $subbanner_padding_left; echo "${subbanner_color}====================================================================================================${RESET_COLOR}"; printf "${subbanner_color}= %*s${RESET_COLOR}%s${subbanner_color}%*s =${RESET_COLOR}\n" $subbanner_padding_left "" "$subbanner_title" $subbanner_padding_right ""; echo "${subbanner_color}====================================================================================================${RESET_COLOR}"; unset subbanner_title subbanner_color subbanner_title_length subbanner_padding subbanner_padding_left subbanner_padding_right'
