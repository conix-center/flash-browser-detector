CC = gcc
CXX = g++
AR = ar

CPPFLAGS = -Iapriltag/
CFLAGS = -g -std=gnu99 -Wall -Wno-unused-parameter -Wno-unused-function -fPIC -O3
CXXFLAGS = -g -Wall -O3
LDFLAGS = -lpthread -lm

TARGETS := apriltag_demo opencv_demo

# do not compile pywarp and some tag families that take a long time to compile
APRILTAG_SRCS := $(shell ls apriltag/*.c apriltag/common/*.c | grep -v -e apriltag_pywrap.c -e tagCircle49h12.c -e tagCustom48h12.c -e tagStandard52h13.c)
APRILTAG_OBJS := $(APRILTAG_SRCS:%.c=%.o)

.PHONY: all
all: apriltag_demo opencv_demo

apriltag_demo: apriltag_demo.o libapriltag.o
	@echo "   [$@]"
	$(CC) -o $@ $^ $(LDFLAGS) 

opencv_demo: opencv_demo.o libapriltag.o
	@echo "   [$@]"
	@$(CXX) -o $@ $^ $(LDFLAGS) `pkg-config --libs opencv`

%.o: %.c
	@echo "   $@"
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

%.o: %.cc
	@echo "   $@"
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $(CPPFLAGS)

libapriltag.o: $(APRILTAG_OBJS)
	@echo "   [$@]"
	$(CC) -shared -o $@ $(APRILTAG_OBJS)

.PHONY: clean
clean:
	@rm -rf *.o apriltag/*.o $(TARGETS)

