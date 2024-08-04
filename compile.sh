#!/bin/sh

DNLOAD="../dnload/dnload.py"
if [ ! -f "${DNLOAD}" ] ; then
    DNLOAD="../faemiyah-demoscene/dnload/dnload.py"
    if [ ! -f "${DNLOAD}" ] ; then
        DNLOAD="/usr/local/src/dnload/dnload.py"
        if [ ! -f "${DNLOAD}" ] ; then
            DNLOAD="/usr/local/src/faemiyah-demoscene/dnload/dnload.py"
            if [ ! -f "${DNLOAD}" ] ; then
                echo "${0}: could not find dnload.py"
                exit 1
            fi
        fi
    fi
fi

if [ ! -f "src/dnload.h" ] ; then
  touch src/dnload.h
fi

python3 "${DNLOAD}" -v src/intro.cpp src/synth.asm --rand=gnu --hash=crc32 --32 -o olkiluoto_3-2-1 $*
if [ $? -ne 0 ] ; then
    echo "${0}: compilation failed"
    exit 1
fi

exit 0
