CC = clang++ -MMD -MP	-std=c++14

SRC_DIR = ./src
SOURCES = $(wildcard $(SRC_DIR)/*.cc)
OBJS = $(SOURCES:.cc=.o)
DEPS = $(SOURCES:.cc=.d)

all: main

main: $(OBJS)
	$(CC) -g -o $@ $(OBJS)

$(OBJS): %.o: %.cc
	$(CC) -c -o $@ $<

clean:
	rm -f main $(OBJS) $(DEPS)

-include $(DEPS)

.PHONY: all clean
