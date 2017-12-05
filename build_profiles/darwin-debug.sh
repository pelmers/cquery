#/bin/bash

echo -g -O3 -std=c++11 -Wall -Wno-sign-compare -Werror -isystem "$(xcode-select -p)"/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1/
