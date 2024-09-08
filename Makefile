VERSION = 1.0.1

BUILD_MODE ?= debug

CC = gcc
LIBS = libevdev libxdo
CFLAGS = -Wall -Wextra `pkg-config --cflags $(LIBS)` -DVERSION=\"$(VERSION)\"
ifeq ($(BUILD_MODE), release)
CFLAGS += -DPROD -DNDEBUG -O3 -flto
else
CFLAGS += -ggdb
endif
LDFLAGS = `pkg-config --libs $(LIBS)` -lm
OBJS = $(patsubst %.c,%.o,$(wildcard src/*.c))
EXEC = desktop-controller

.PHONY: all version clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

-include $(OBJS:.o=.d)

%.o: %.c
	gcc $(CFLAGS) -MMD -MP -c -o $@ $<

version:
	@echo $(VERSION)

clean:
	rm --force --verbose $(EXEC) $(OBJS) $(OBJS:.o=.d)
