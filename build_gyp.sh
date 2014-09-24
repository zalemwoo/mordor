#!/bin/sh

mkdir -p out
cd out && ../gyp/gyp/gyp ../gyp/mordor.gyp --depth=. -D target_arch=x64 --no-duplicate-basename-check
