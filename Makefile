NAME=desktop-controller
VERSION=1.0.0

CC=gcc
LIBS=libxdo libevdev
CFLAGS=-Wall -Wextra `pkg-config --cflags $(LIBS)`
CFLAGS += -DVERSION=\"$(VERSION)\"
ifeq ($(PROD), 1)
CFLAGS += -DPROD -DNDEBUG -O3
else
CFLAGS += -ggdb
endif
LDFLAGS=`pkg-config --libs $(LIBS)` -lm
EXEC=$(NAME)

SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

.PHONY: all version clean

all: $(EXEC)

src/*.o: src/*.h
%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(EXEC): $(OBJS)
	$(CC) -o $@ src/*.o $(LDFLAGS)

version:
	@echo $(VERSION)

clean:
	rm --force --verbose $(EXEC) src/*.o
