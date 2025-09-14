BUILD_DIR := build

.PHONY: all run test clean

all: run

$(BUILD_DIR)/Makefile:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. > /dev/null

run: $(BUILD_DIR)/Makefile
	@cd $(BUILD_DIR) && $(MAKE) -s venus
	@./$(BUILD_DIR)/venus

test: $(BUILD_DIR)/Makefile
	@cd $(BUILD_DIR) && $(MAKE) -s test
	@./$(BUILD_DIR)/test

clean:
	@rm -rf $(BUILD_DIR)
