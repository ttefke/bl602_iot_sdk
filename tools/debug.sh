if ! command -v gnome-terminal >/dev/null 2>&1
then
    echo "gnome-terminal could not be found, can not run program"
    exit 1
fi

path=$(pwd)
wd=$(basename "$path")
upd=$(basename "$(dirname "$path")")

if [[ "$upd" != "customer_app" ]]; then
    echo "Script must be started from the application's source code root directory"
    exit 1
fi

echo "Starting openOCD"
gnome-terminal --  bash -c "$path/../../toolchain/openocd/bin/openocd -f $path/../../tools/debug/openocd.cfg; exec bash"

echo "Creating GDB configuration"

cat > /tmp/gdb_tgt.cfg << EOF
target extended-remote :3333
EOF


cat $(pwd)/../../tools/debug/602.init /tmp/gdb_tgt.cfg > /tmp/602.init

echo "Starting GDB"
gnome-terminal -- bash -c "$path/../../toolchain/compiler/bin/riscv32-unknown-elf-gdb -x /tmp/602.init --se=$path/build_out/$wd.elf; exec bash"
