#!/bin/bash
prompt="Pick the target platform:"
chip_options=("a33" \
              "h8" \
              "h3" \
              "rk3036" \
              "rk3126" \
              "rk3128" \
              "rk322x" \
              "atm7039-action" \
              "aml-s905" \
              "aml-s805" \
              "x1000" \
              "t10" \
              "xml-hi3518")
PLATFORM=""

select opt in "${chip_options[@]}" "Quit"; do
    case "$REPLY" in

    1 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    2 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    3 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    4 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    5 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    6 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    7 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    8 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    9 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    10 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    11 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    12 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    13 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;

    $(( ${#chip_options[@]}+1 )) ) echo "Goodbye!"; break;;
    *) echo "Invalid option. Try another one.";continue;;
    esac
done

if [ "$PLATFORM" != "" ]; then
./ver_info.pl include/ssv_version.h

if [ $? -eq 0 ]; then
    echo "Please check SVN first !!"
else
mv Makefile Makefile.org
cp Makefile.prealloc Makefile
sed -i 's,PLATFORMS =,PLATFORMS = '"$PLATFORM"',g' Makefile
make clean
make
cp Makefile.android Makefile
sed -i 's,PLATFORMS =,PLATFORMS = '"$PLATFORM"',g' Makefile
make
mv -f Makefile.org Makefile
echo "Done ko!"
fi
else
echo "Fail!"
fi

