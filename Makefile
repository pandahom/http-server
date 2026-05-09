CC 				:= gcc
CFLAGS 			:= -Werror -Wall -Wextra -c -g

BUILD_DIR 	 	:= build
SRC_DIR 	 	:= src

SRC 			:= 	$(SRC_DIR)/server-sm.c \
					$(SRC_DIR)/main.c   \
				   $(SRC_DIR)/server.c \
				   $(SRC_DIR)/conn-client.c \
				   $(SRC_DIR)/connection-sm.c
 HEADER_FILES	:= $(SRC_DIR)/common.h \
				   $(SRC_DIR)/server-sm.h \
				   $(SRC_DIR)/server.h \
				   $(SRC_DIR)/connection-sm.h \
				   $(SRC_DIR)/conn-client.h


OBJ	   := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))
DS_OBJ := $(patsubst $(DS_DIR)/%.c, $(BUILD_DIR)/%.o, $(DS))

TARGET := serve-file

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	@echo "Created build directory: $(BUILD_DIR)"


$(TARGET): $(OBJ) $(DS_OBJ) $(HEADER_FILES)
	@echo "Linking object files into $(TARGET)..."
	@$(CC) $(OBJ) $(DS_OBJ) -o $@
	@echo "Build complete. Executable: ./$(TARGET)"

$(BUILD_DIR)/%.o: src/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: src/ds/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	@echo "Cleaning up buid files..."
	@rm -f $(OBJ) 2> /dev/null
	@rm -rf $(BUILD_DIR) 2> /dev/null
	@rm $(TARGET) 2> /dev/null
	@echo "Clean up complete."
