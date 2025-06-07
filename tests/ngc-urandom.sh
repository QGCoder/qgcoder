#!/usr/bin/env bash

N=$1; echo "F100"; cat /dev/urandom | hexdump -v -e '/1 "%u\n"' | paste - - -  | awk '{ print "G1 X"$1" Y"$2" Z"$3 }'|head -n $N; echo "M30"
