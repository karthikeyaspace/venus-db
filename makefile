BUILD_DIR := build

.PHONY: all run test clean

all: run

$(BUILD_DIR)/Makefile:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. > /dev/null

run: $(BUILD_DIR)/Makefile
	@cd $(BUILD_DIR) && $(MAKE) -s venus
	@./$(BUILD_DIR)/venus

test:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -DBUILD_TESTS=ON > /dev/null
	@cd $(BUILD_DIR) && $(MAKE) -s test_suite
	@./$(BUILD_DIR)/test_suite

clean:
	@rm -rf $(BUILD_DIR)
