#! /bin/sh

c++ `llvm-config --cxxflags --ldflags --system-libs --libs core` -std=c++20 -fexceptions -Wall -Wextra -Werror cpp/main.cpp -o brmh
