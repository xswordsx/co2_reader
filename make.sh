#!/bin/env bash

set -e

if [ ! -d build/ ] ; then mkdir build/ ; fi

gcc -s -Wall ./src/main.c -I./include -o build/co2_reader $*
