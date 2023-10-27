#!/usr/bin/bash

rm -r build bin
cmake -S . -B build
cd build
make