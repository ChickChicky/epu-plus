#!/usr/bin/env sh

clang --target=wasm32 -Wall -Wextra -O0 --no-standard-libraries -Wl,--allow-undefined -Wl,--export-all -Wl,--no-entry -o epu.wasm epu.c