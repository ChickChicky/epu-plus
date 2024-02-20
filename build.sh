#!/usr/bin/env sh

clang --target=wasm32 -Wall -Wextra -O3 --no-standard-libraries -fno-builtin -mbulk-memory -Wl,--allow-undefined -Wl,--export-all -Wl,--no-entry -o epu.wasm epu.c