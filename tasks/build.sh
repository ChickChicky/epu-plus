#!/usr/bin/env sh

## Builds the C part of the project to WASM ##
set -xe

clang --target=wasm32 -Wall -Wextra -Ofast --no-standard-libraries -fno-builtin -mbulk-memory -Wl,--allow-undefined -Wl,--export-all -Wl,--no-entry -o ./epu.wasm ./src/epu-c/epu.c