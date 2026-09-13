#!/bin/bash

clang++ --analyze -std=c++17 -Iinclude ./include/castle/*.hpp
clang++ --analyze -std=c++17 -Iinclude ./include/castle/*/*.hpp
clang++ --analyze -std=c++17 -Iinclude ./include/castle/*/*/*.hpp

find include/castle -type f -name '*.gch' -delete
