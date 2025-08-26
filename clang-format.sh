#!/usr/bin/env bash

find MB85RC256V-pico/ -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i
