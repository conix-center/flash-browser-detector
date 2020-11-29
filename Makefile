CC 					= gcc
CXX 				= g++
EMCC 				= emcc

BIN_DIR 			= bin
OBJ_DIR 			= obj
HTML_BIN_DIR 		= html/$(BIN_DIR)

GLITTER_DIR 		= glitter
APRILTAG_DIR 		= apriltag
EXAMPLES_DIR		= examples
WASM_DIR 			= wasm

INCLUDE 			= -I$(APRILTAG_DIR)/ -I$(GLITTER_DIR)/
C_FLAGS 			= -g -std=gnu99 -Wall -Wno-unused-parameter -Wno-unused-function -O3
CXX_FLAGS			= -g -Wall -O3 -std=c++11
LD_FLAGS 			= -lpthread -lm

WASM_MODULE_NAME 	= GlitterWASM
WASM_FLAGS			= -Wall -O3
WASM_LD_FLAGS 		= -s EXPORT_NAME='$(WASM_MODULE_NAME)' -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='['_malloc', '_free']' -s EXTRA_EXPORTED_RUNTIME_METHODS='['cwrap', 'getValue', 'setValue']' -s WASM=1

OPENCV_C_FLAGS		= `pkg-config --cflags opencv`
OPENCV_LD_FLAGS		= `pkg-config --libs opencv`

# do not compile pywarp and some tag families that take a long time to compile
APRILTAG_SRCS 		:= $(shell ls $(APRILTAG_DIR)/*.c $(APRILTAG_DIR)/common/*.c | grep -v -e apriltag_pywrap.c -e tagCircle49h12.c -e tagCustom48h12.c -e tagStandard52h13.c)
APRILTAG_OBJS 		:= $(APRILTAG_SRCS:%.c=%.o)

GLITTER_SRCS 		:= $(wildcard $(GLITTER_DIR)/*.c)
GLITTER_OBJS 		:= $(GLITTER_SRCS:$(GLITTER_DIR)/%.c=$(OBJ_DIR)/%.o)

EXAMPLES_SRCS		:= $(wildcard $(EXAMPLES_DIR)/*.c $(EXAMPLES_DIR)/*.cpp)
EXAMPLES_OBJS		:= $(EXAMPLES_SRCS:$(EXAMPLES_DIR)/%.c=$(OBJ_DIR)/%.o) $(EXAMPLES_SRCS:$(EXAMPLES_DIR)/%.cpp=$(OBJ_DIR)/%.o)
EXAMPLES_TARGETS	:= $(EXAMPLES_SRCS:$(EXAMPLES_DIR)/%.c=$(BIN_DIR)/%) $(EXAMPLES_SRCS:$(EXAMPLES_DIR)/%.cpp=$(BIN_DIR)/%)

WASM_SRCS			:= $(wildcard $(WASM_DIR)/*.c)
WASM_TARGETS		:= $(WASM_SRCS:$(WASM_DIR)/%.c=$(HTML_BIN_DIR)/%.js)

.PHONY: all clean

all: 		$(EXAMPLES_TARGETS) $(WASM_TARGETS)
examples: 	$(EXAMPLES_TARGETS)
wasm: 		$(WASM_TARGETS)

$(BIN_DIR)/apriltag_demo: $(OBJ_DIR)/apriltag_demo.o $(APRILTAG_OBJS)
	@echo "=================================================="
	@echo "    Linking target [$@]"
	@$(CC) -o $@ $^ $(LD_FLAGS)

$(BIN_DIR)/apriltag_quads: $(OBJ_DIR)/apriltag_quads.o $(GLITTER_OBJS) $(APRILTAG_OBJS)
	@echo "=================================================="
	@echo "    Linking target [$@]"
	@$(CC) -o $@ $^ $(LD_FLAGS)

$(BIN_DIR)/lightanchor_demo: $(OBJ_DIR)/lightanchor_demo.o $(GLITTER_OBJS) $(APRILTAG_OBJS)
	@echo "=================================================="
	@echo "    Linking target [$@]"
	@$(CXX) -o $@ $^ $(LD_FLAGS) $(OPENCV_LD_FLAGS)

$(BIN_DIR)/opencv_demo: $(OBJ_DIR)/opencv_demo.o $(APRILTAG_OBJS)
	@echo "=================================================="
	@echo "    Linking target [$@]"
	@$(CXX) -o $@ $^ $(LD_FLAGS) $(OPENCV_LD_FLAGS)

$(BIN_DIR)/webcam_quads: $(OBJ_DIR)/webcam_quads.o $(GLITTER_OBJS) $(APRILTAG_OBJS)
	@echo "=================================================="
	@echo "    Linking target [$@]"
	@$(CXX) -o $@ $^ $(LD_FLAGS) $(OPENCV_LD_FLAGS)

$(BIN_DIR)/webcam_lightanchors: $(OBJ_DIR)/webcam_lightanchors.o $(GLITTER_OBJS) $(APRILTAG_OBJS)
	@echo "=================================================="
	@echo "    Linking target [$@]"
	@$(CXX) -o $@ $^ $(LD_FLAGS) $(OPENCV_LD_FLAGS)

$(APRILTAG_DIR)/%.o: $(APRILTAG_DIR)/%.c | $(BIN_DIR) $(OBJ_DIR)
	@echo "=================================================="
	@echo "    Compiling apriltag target [$<]"
	@$(CC) -o $@ -c $< $(C_FLAGS) $(INCLUDE)

$(OBJ_DIR)/%.o: $(GLITTER_DIR)/%.c | $(BIN_DIR) $(OBJ_DIR)
	@echo "=================================================="
	@echo "    Compiling GLITTER target [$<]"
	@$(CC) -o $@ -c $< $(C_FLAGS) $(INCLUDE)

$(OBJ_DIR)/%.o: $(EXAMPLES_DIR)/%.c | $(BIN_DIR) $(OBJ_DIR)
	@echo "=================================================="
	@echo "    Compiling target [$<]"
	@$(CC) -o $@ -c $< $(C_FLAGS) $(INCLUDE)

$(OBJ_DIR)/%.o: $(EXAMPLES_DIR)/%.cpp | $(BIN_DIR) $(OBJ_DIR)
	@echo "=================================================="
	@echo "    Compiling target [$<]"
	@$(CXX) -o $@ -c $< $(CXX_FLAGS) $(INCLUDE) $(OPENCV_C_FLAGS)

$(HTML_BIN_DIR)/%.js: $(WASM_DIR)/%.c $(APRILTAG_SRCS) $(GLITTER_SRCS) | $(HTML_BIN_DIR) $(HTML_OBJ_DIR)
	@echo "=================================================="
	@echo "    Compiling WASM target [$<]"
	@echo "    Be sure to clone emsdk and run 'source ./emsdk/emsdk_env.sh'!"
	@$(EMCC) -o $@ $^ $(INCLUDE) $(WASM_FLAGS) $(WASM_LD_FLAGS)

$(BIN_DIR) $(OBJ_DIR) $(HTML_BIN_DIR):
	@mkdir $@

clean:
	@rm -rf $(BIN_DIR) $(OBJ_DIR) $(HTML_BIN_DIR) $(APRILTAG_DIR)/*.o $(APRILTAG_DIR)/common/*.o build *.pnm *.ps
