CC = gcc
CXX = g++
AR = ar

CPPFLAGS = -Iapriltag/
CFLAGS = -g -std=gnu99 -Wall -Wno-unused-parameter -Wno-unused-function -O3
CXXFLAGS = -g -Wall -O3
LDFLAGS = -lpthread -lm 

TARGETS := apriltag_demo apriltag_quads opencv_demo

# do not compile pywarp and some tag families that take a long time to compile
APRILTAG_SRCS := $(shell ls apriltag/*.c apriltag/common/*.c | grep -v -e apriltag_pywrap.c -e tagCircle49h12.c -e tagCustom48h12.c -e tagStandard52h13.c)
APRILTAG_OBJS := $(APRILTAG_SRCS:%.c=%.o)

.PHONY: all
all: apriltag_demo apriltag_quads opencv_demo

apriltag_demo: apriltag_demo.o $(APRILTAG_OBJS)
	@echo "   [$@]"
	@$(CC) -o $@ $^ $(LDFLAGS) 

apriltag_quads: apriltag_quads.o lightanchor_detector.o $(APRILTAG_OBJS) 
	@echo "   [$@]"
	@$(CC) -o $@ $^ $(LDFLAGS) 

opencv_demo: opencv_demo.o $(APRILTAG_OBJS) 
	@echo "   [$@]"
	@$(CXX) -o $@ $^ $(LDFLAGS) `pkg-config --libs opencv`

%.o: %.c
	@echo "   $@"
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

%.o: %.cc
	@echo "   $@"
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $(CPPFLAGS)

#libapriltag.o: $(APRILTAG_OBJS)
#	@echo "   [$@]"
#	@$(CC) -o $@ $(APRILTAG_OBJS)

.PHONY: clean
clean:
	@rm -rf *.o $(TARGETS)

