#!/bin/bash

clang-tidy \
    -checks='-*,bugprone-*,cert-*,cppcoreguidelines-*,performance-*,portability-*' \
    -header-filter='include/castle/.*' \
    include/castle/*.h \
    -- \
    -x c++ \
    -I include \
    -std=c++17

