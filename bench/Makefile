ROOT := ..
# Benchmarks are meaningless under sanitizers, so default to release.
MODE ?= release
include $(ROOT)/config.mk

BINS := $(patsubst %.c,$(BUILD_DIR)/%,$(wildcard *.c))

all: $(BINS)
	@for b in $(BINS); do printf '== %s\n' "$$b"; ./$$b || exit 1; done

$(BUILD_DIR)/%: %.c $(LIB_SRCS)
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $< $(LIB_SRCS) -o $@ $(LDFLAGS)

clean:
	@rm -rf build
.PHONY: all clean
