# Name of the module to build
obj-m := ku_copy_bench.o

# Location of the kernel build directory
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

# Check if the kernel was built with Clang (look for CC=clang in .config or version string)
ifeq ($(shell test -f $(KERNEL_DIR)/.config && grep -q '^CONFIG_CC_IS_CLANG=y' $(KERNEL_DIR)/.config && echo clang),clang)
    IS_CLANG_KERNEL := 1
else ifeq ($(shell test -f $(KERNEL_DIR)/Makefile && grep -q '^LLVM ' $(KERNEL_DIR)/Makefile),LLVM )
    # Newer kernels expose LLVM=1 in top-level Makefile
    IS_CLANG_KERNEL := 1
endif

ifdef IS_CLANG_KERNEL
    KBUILD_OPTIONS += LLVM=1
    # avoid ThinLTO cache permission hell + faster builds
    KBUILD_OPTIONS += LTO=none
    # Optional: silence compiler mismatch warning
    KBUILD_OPTIONS += IGNORE_COMPILER_MISMATCH=1
    $(info === Detected Clang-built kernel → using LLVM=1 LTO=none)
endif

# Default target: build the module
all:
	$(MAKE) $(KBUILD_OPTIONS) -C $(KERNEL_DIR) M=$(PWD) modules

# Clean target
clean:
	$(MAKE) $(KBUILD_OPTIONS) -C $(KERNEL_DIR) M=$(PWD) clean
