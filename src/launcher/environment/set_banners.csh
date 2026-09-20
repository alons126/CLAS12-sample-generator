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
alias code_banner 'set banner_title_length = `printf "%s" "$banner_title" | wc -c`; @ banner_padding = 96 - $banner_title_length; @ banner_padding_left = $banner_padding / 2; @ banner_padding_right = $banner_padding - $banner_padding_left; echo "${banner_color}////////////////////////////////////////////////////////////////////////////////////////////////////${COLOR_END}"; printf "${banner_color}//%*s${COLOR_END}%s${banner_color}%*s//${COLOR_END}\n" $banner_padding_left "" "$banner_title" $banner_padding_right ""; echo "${banner_color}////////////////////////////////////////////////////////////////////////////////////////////////////${COLOR_END}"; unset banner_title banner_color banner_title_length banner_padding banner_padding_left banner_padding_right'

# Render every titled banner at 100 visible columns. Callers supply banner_title and banner_color;
# the helper calculates asymmetric padding when an odd number of spaces is required.
alias code_subbanner 'set banner_title_length = `printf "%s" "$banner_title" | wc -c`; @ banner_padding = 96 - $banner_title_length; @ banner_padding_left = $banner_padding / 2; @ banner_padding_right = $banner_padding - $banner_padding_left; echo "${banner_color}====================================================================================================${COLOR_END}"; printf "${banner_color}= %*s${COLOR_END}%s${banner_color}%*s =${COLOR_END}\n" $banner_padding_left "" "$banner_title" $banner_padding_right ""; echo "${banner_color}====================================================================================================${COLOR_END}"; unset banner_title banner_color banner_title_length banner_padding banner_padding_left banner_padding_right'
