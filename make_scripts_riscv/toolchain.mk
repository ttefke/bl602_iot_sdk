##change to your toolchain path
ifneq ($(shell uname),Linux)
CONFIG_TOOLPREFIX ?= $(BL60X_SDK_PATH)/toolchain/riscv/$(shell uname |cut -d '_' -f1)/bin/riscv64-unknown-elf-
else
CONFIG_TOOLPREFIX ?= $(BL60X_SDK_PATH)/toolchain/riscv/$(shell uname |cut -d '_' -f1)/gcc/bin/riscv64-unknown-elf-
endif
#CONFIG_TOOLPREFIX ?= riscv64-unknown-elf-
