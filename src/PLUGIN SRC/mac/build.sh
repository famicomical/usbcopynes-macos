#!/bin/sh
# Build a TASM-syntax plugin from DONE/ on macOS with cc65:  ./build.sh TXROM256
# Needs: brew install cc65
set -e
cd "$(dirname "$0")"
python3 tasm2ca65.py "../DONE/$1.ASM" "/tmp/$1.s"
ca65 "/tmp/$1.s" -o "/tmp/$1.o"
ld65 -C plugin.cfg -o "../../../bin/PLUGINS/$(echo "$1" | tr 'A-Z' 'a-z').bin" "/tmp/$1.o"
