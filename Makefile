CC = gcc
CXX = g++

BIN_DIR = bin
OBJ_DIR = obj

GLITTER_DIR 		= glitter
APRILTAG_DIR 		= apriltag

CPPFLAGS 			= -I$(APRILTAG_DIR)/ -I$(GLITTER_DIR)/
CFLAGS 				= -g -std=gnu99 -Wall -Wno-unused-parameter -Wno-unused-function -O3
CXXFLAGS			= -g -Wall -O3 -std=c++11
LDFLAGS 			= -lpthread -lm

OPENCV_CFLAGS		= `pkg-config --cflags opencv`
OPENCV_LDFLAGS		= `pkg-config --libs opencv`

# do not compile pywarp and some tag families that take a long time to compile
APRILTAG_SRCS 		:= $(shell ls $(APRILTAG_DIR)/*.c $(APRILTAG_DIR)/common/*.c | grep -v -e apriltag_pywrap.c -e tagCircle49h12.c -e tagCustom48h12.c -e tagStandard52h13.c)
APRILTAG_OBJS 		:= $(APRILTAG_SRCS:%.c=%.o)

GLITTER_SRCS 		:= $(wildcard $(GLITTER_DIR)/*.c)
GLITTER_OBJS 		:= $(GLITTER_SRCS:$(GLITTER_DIR)/%.c=$(OBJ_DIR)/%.o)

TARGETS 			:= $(BIN_DIR)/apriltag_demo $(BIN_DIR)/apriltag_quads $(BIN_DIR)/opencv_demo

.PHONY: all clean

all: $(TARGETS)

$(BIN_DIR)/apriltag_demo: $(OBJ_DIR)/apriltag_demo.o $(APRILTAG_OBJS)
	@echo "======================="
	@echo "    Linking target [$@]"
	@$(CC) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/apriltag_quads: $(OBJ_DIR)/apriltag_quads.o $(GLITTER_OBJS) $(APRILTAG_OBJS)
	@echo "======================="
	@echo "    Linking target [$@]"
	@$(CC) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/opencv_demo: $(OBJ_DIR)/opencv_demo.o $(APRILTAG_OBJS)
	@echo "======================="
	@echo "    Linking target [$@]"
	@$(CXX) -o $@ $^ $(LDFLAGS) $(OPENCV_LDFLAGS)

$(OBJ_DIR)/%.o: $(GLITTER_DIR)/%.c | $(BIN_DIR) $(OBJ_DIR)
	@echo "======================="
	@echo "    Compiling GLITTER target [$<]"
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

$(OBJ_DIR)/%.o: %.c | $(BIN_DIR) $(OBJ_DIR)
	@echo "======================="
	@echo "    Compiling target [$<]"
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

$(OBJ_DIR)/%.o: %.cpp | $(BIN_DIR) $(OBJ_DIR)
	@echo "======================="
	@echo "    Compiling target [$<]"
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $(CPPFLAGS) $(OPENCV_CFLAGS)

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BIN_DIR) $(OBJ_DIR) build *.pnm *.ps
