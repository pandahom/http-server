CC 				:= gcc
CFLAGS 			:= -Werror -Wall -Wextra -c -g

BUILD_DIR 	 	:= build
SRC_DIR 	 	:= src
DS_DIR			:= src/ds
HTML_PAGES		:= src/static-response-bodies

DS 				:= $(DS_DIR)/linked-list.c \
				   $(DS_DIR)/ht.c

SRC 			:= 	$(SRC_DIR)/server-sm.c \
					$(SRC_DIR)/main.c   \
				   $(SRC_DIR)/server.c \
				   $(SRC_DIR)/conn-client.c \
				   $(SRC_DIR)/connection-sm.c \
				   $(SRC_DIR)/request-handler.c \
				   $(SRC_DIR)/response-build.c \
				   $(SRC_DIR)/path-handler.c \

 HEADER_FILES	:= $(SRC_DIR)/common.h \
				   $(SRC_DIR)/server-sm.h \
				   $(SRC_DIR)/server.h \
				   $(SRC_DIR)/connection-sm.h \
				   $(SRC_DIR)/conn-client.h \
				   $(SRC_DIR)/request-handler.h \
				   $(SRC_DIR)/response-build.h \
				   $(SRC_DIR)/path-handler.h \
				   $(SRC_DIR)/path-handler.h \
				   $(HTML_PAGES)/http_400.h \
				   $(HTML_PAGES)/http_404.h \
				   $(HTML_PAGES)/http_500.h \
				   $(HTML_PAGES)/http_505.h \
				   $(HTML_PAGES)/http_501.h \
				   $(HTML_PAGES)/default_dir_list_page.h \
				   $(DS_DIR)/linked-list.h \
				   $(DS_DIR)/ht.h


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
