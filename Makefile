BUILD_DIR = build
CMAKE = cmake
CMAKE_FLAGS = -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$(shell pwd)
TARGET = joosc

all: $(TARGET)

$(BUILD_DIR)/Makefile:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) ..

$(TARGET): $(BUILD_DIR)/Makefile
	cd $(BUILD_DIR) && $(MAKE) $(TARGET)
	cp $(BUILD_DIR)/$(TARGET) .

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean $(TARGET)
