UwUMaker-name = FluffyGC
UwUMaker-is-executable := m

UwUMaker-c-flags-y += -std=c2x -g \
	-Wall -Wshadow -Wpointer-arith \
	-Wmissing-prototypes \
	-fpic \
	-fblocks -Wextra \
	-D_POSIX_C_SOURCE=200809L \
	-I$(PROJECT_DIR)/deps/list \
	-I$(PROJECT_DIR)/deps/buffer \
	-I$(PROJECT_DIR)/deps/vec \
	-I$(PROJECT_DIR)/deps/templated-hashmap \
	-I$(PROJECT_DIR)/src \
	-I$(PROJECT_DIR)/include \
	-fblocks -fvisibility=hidden -fno-common \

UwUMaker-linker-flags-y += -lm -lBlocksRuntime
UwUMaker-dirs-y += src deps

# Stacktrace options
UwUMaker-linker-flags-$(CONFIG_STACKTRACE_PROVIDER_LIBBACKTRACE) += -lbacktrace
UwUMaker-pkg-config-libs-$(CONFIG_STACKTRACE_PROVIDER_LIBUNWIND) += libunwind



