ROOT := ..
include $(ROOT)/config.mk

# One executable per unit-test file, linked against the compiler minus its main().
BINS := $(patsubst %.c,$(BUILD_DIR)/%,$(wildcard base/*.c))

all: $(BINS)
	@fail=0; for t in $(BINS); do printf '== %s\n' "$$t"; ./$$t || fail=1; done; \
	 if [ -x run.sh ]; then ./run.sh || fail=1; fi; \
	 exit $$fail

$(BUILD_DIR)/%: %.c greatest.h $(LIB_SRCS)
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $< $(LIB_SRCS) -o $@ $(LDFLAGS)

clean:
	@rm -rf build
.PHONY: all clean
