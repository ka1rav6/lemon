# Shared build configuration, included by ./Makefile, tests/Makefile and bench/Makefile.
# Each includer sets ROOT to the path from itself to the repository root.

ROOT ?= .
CC   := gcc

# debug (default) or release; `make release` and bench/ set this.
MODE ?= debug

CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -I$(ROOT)/src

ifeq ($(MODE),debug)
  CFLAGS  += -g -O0 -DDEBUG_MODE=1 -fsanitize=address,undefined
  LDFLAGS += -fsanitize=address,undefined
else ifeq ($(MODE),release)
  CFLAGS  += -O2 -DDEBUG_MODE=0
else
  $(error MODE must be debug or release, not '$(MODE)')
endif

BUILD_DIR := build/$(MODE)

# Compiler sources; LIB_SRCS is what tests and benchmarks link against.
SRCS     := $(patsubst ./%,%,$(shell find $(ROOT)/src -name "*.c" 2>/dev/null))
LIB_SRCS := $(filter-out $(ROOT)/src/main.c,$(SRCS))
