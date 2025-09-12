#!/bin/bash

path=$(pwd)
wd=$(basename "$path")
upd=$(basename "$(dirname "$path")")

if [[ "$upd" != "customer_app" ]]; then
    echo "Script must be started from the application's source code root directory"
    exit 1
fi

../../toolchain/compiler/bin/riscv32-unknown-elf-objdump build_out/$wd.elf -x -s -S --disassemble --demangle --line-numbers --wide > $wd.S 2>&1
