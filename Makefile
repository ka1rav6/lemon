include config.mk 
# the above includes all the basic configurations : things like flags, debug mode logic etc

BIN  := $(BUILD_DIR)/lem
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

all: $(BIN)
# recursively calls the makefile with the mode set as debug
debug:
	@$(MAKE) MODE=debug

release:
	$(MAKE) MODE=release

$(BIN): $(OBJS)
	@test -n "$(OBJS)" || { echo "nothing to build: no .c files under src/"; exit 1; }
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# recursively calls the Makefile in the `test` and `bench` folders
test:
	@$(MAKE) -C tests MODE=$(MODE)

bench:
	@$(MAKE) -C bench

clean:
	@rm -rf build
	@$(MAKE) -C tests clean
	@$(MAKE) -C bench clean

-include $(OBJS:.o=.d)
.PHONY: all debug release test bench clean
