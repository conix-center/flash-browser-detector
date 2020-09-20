CC = gcc
CXX = g++

INC_DIR = include
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj

CPPFLAGS = -Iapriltag/ -I$(INC_DIR)/
CFLAGS = -g -std=gnu99 -Wall -Wno-unused-parameter -Wno-unused-function -O3
CXXFLAGS = -g -Wall -O3
LDFLAGS = -lpthread -lm

TARGETS := $(BIN_DIR)/apriltag_demo $(BIN_DIR)/apriltag_quads

# do not compile pywarp and some tag families that take a long time to compile
APRILTAG_SRCS := $(shell ls apriltag/*.c apriltag/common/*.c | grep -v -e apriltag_pywrap.c -e tagCircle49h12.c -e tagCustom48h12.c -e tagStandard52h13.c)
APRILTAG_OBJS := $(APRILTAG_SRCS:%.c=%.o)

LIGHTANCHOR_SRCS := $(wildcard $(SRC_DIR)/*.c)
LIGHTANCHOR_OBJS := $(LIGHTANCHOR_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean

all: $(TARGETS)

$(BIN_DIR)/apriltag_demo: $(OBJ_DIR)/apriltag_demo.o $(APRILTAG_OBJS)
	@echo "==============="
	@echo "   Linking target [$@]"
	@$(CC) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/apriltag_quads: $(OBJ_DIR)/apriltag_quads.o $(LIGHTANCHOR_OBJS) $(APRILTAG_OBJS)
	@echo "==============="
	@echo "   Linking target [$@]"
	@$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR) $(OBJ_DIR)
	@echo "==============="
	@echo "   Compiling GLITTER target [$<]"
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

$(OBJ_DIR)/%.o: %.c | $(BIN_DIR) $(OBJ_DIR)
	@echo "==============="
	@echo "   Compiling target [$<]"
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BIN_DIR) $(OBJ_DIR)
