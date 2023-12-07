UwUMaker-dirs-y += src

UwUMaker-is-executable = m

# Deps
UwUMaker-c-sources-y += deps/list/list_node.c \
	deps/list/list.c \
	deps/list/list_iterator.c \
	deps/vec/vec.c \
	deps/buffer/buffer.c \
	deps/templated-hashmap/hashmap.c

UwUMaker-c-flags-y += -std=c2x \
	-D_POSIX_C_SOURCE=200809L \
	-I$(PROJECT_DIR)/deps/list \
	-I$(PROJECT_DIR)/deps/buffer \
	-I$(PROJECT_DIR)/deps/vec \
	-I$(PROJECT_DIR)/deps/templated-hashmap \
	-I$(PROJECT_DIR)/src \
	-I$(PROJECT_DIR)/include \
	-fblocks -fvisibility=hidden -fno-common
UwUMaker-linker-flags-y += -lm \
	-fvisibility=hidden -fno-common

