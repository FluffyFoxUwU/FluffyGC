
UwUMaker-name := AWholeSmallVMForTesting
UwUMaker-is-executable := y

UwUMaker-c-sources-y += main.c
UwUMaker-c-flags-y += -isystem "$(ROOT_PROJECT_DIR)/api/headers" -std=c23
UwUMaker-linker-tail-flags-y += -lmimalloc -lFlup -lBlocksRuntime

UwUMaker-shared-lib-subprojects-y += $(ROOT_PROJECT_DIR)

