UwUMaker-dirs-y += test memory object heap util gc

UwUMaker-c-flags-y += -std=c2x -g -O0 \
		-Wall -Wshadow -Wpointer-arith \
		-Wmissing-prototypes \
		-fpic -fblocks -Wextra \
		-D_POSIX_C_SOURCE=200809L \
		-fvisibility=hidden -fno-common \
		-Wmissing-field-initializers \
		-Wstrict-prototypes \
		-Waddress -Wconversion -Wunused \
		-Wcast-align -Wfloat-equal -Wformat=2 \
		-fstrict-flex-arrays=3 -Warray-bounds \
		-Wno-initializer-overrides \
		-Wundef -fno-omit-frame-pointer

UwUMaker-c-flags-y += -flto=full -O3
UwUMaker-linker-flags-y += -flto=full -O3
# UwUMaker-c-flags-y += -fsanitize=address
# UwUMaker-linker-flags-y += -fsanitize=address
UwUMaker-c-flags-y += -fsanitize=undefined
UwUMaker-linker-flags-y += -fsanitize=undefined
UwUMaker-linker-flags-y += -lFlup -lBlocksRuntime

UwUMaker-pkg-config-libs-y += mimalloc

UwUMaker-is-executable := y
UwUMaker-name := FluffyGC

proj_run:
	@$(MAKE) -C $(UWUMAKER_DIR) PROJECT_DIR="$(PROJECT_DIR)" cmd_all
	@cd "$(PROJECT_DIR)" && $(BUILD_DIR)/objs/FluffyGC

proj_run_gdb:
	@$(MAKE) -C $(UWUMAKER_DIR) PROJECT_DIR="$(PROJECT_DIR)" cmd_all
	@cd "$(PROJECT_DIR)" && gdb $(BUILD_DIR)/objs/FluffyGC


proj_run_valgrind:
	@$(MAKE) -C $(UWUMAKER_DIR) PROJECT_DIR="$(PROJECT_DIR)" cmd_all
	@cd "$(PROJECT_DIR)" && valgrind $(RUN_FLAGS) $(BUILD_DIR)/objs/FluffyGC

