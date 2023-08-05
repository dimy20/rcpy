CC = clang

SRC_DIR = ./src
BUILD_DIR = ./build
INCLUDE_DIRS = ./include
BIN_DIR = ./bin

SERVER_EXEC = $(BIN_DIR)/server
SERVER_SRCS = $(shell find $(SRC_DIR)/server -name '*.c')
SERVER_OBJS = $(patsubst $(SRC_DIR)/server/%.c, $(BUILD_DIR)/server/%.o, $(SERVER_SRCS))

CLIENT_EXEC = $(BIN_DIR)/client
CLIENT_SRCS = $(shell find $(SRC_DIR)/client -name '*.c')
CLIENT_OBJS = $(patsubst $(SRC_DIR)/client/%.c, $(BUILD_DIR)/client/%.o, $(CLIENT_SRCS))

COMMON_SRCS = $(shell find $(SRC_DIR) -maxdepth 1 -name '*.c')
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(COMMON_SRCS))

CFLAGS = -Wall -Werror -std=c11 -g $(foreach D, $(INCLUDE_DIRS), -I$(D))
LIBS = -lpthread

all: server client

print_common:
	$(foreach c, $(COMMON_OBJS), @echo $(c))

server: $(SERVER_EXEC)
client: $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/server/%.o: $(SRC_DIR)/server/%.c
		$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_EXEC): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/client/%.o: $(SRC_DIR)/client/%.c
		$(CC) $(CFLAGS) -c $< -o $@

#common objs
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
		$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/server/* $(BUILD_DIR)/client/* $(BUILD_DIR)/*.o $(BIN_DIR)/*

.PHONY:
	print_common
