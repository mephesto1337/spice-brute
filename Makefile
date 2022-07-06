INCLUDES = \
   $(shell pkg-config --cflags glib-2.0) \
   $(shell pkg-config --cflags spice-client-glib-2.0) \
   $(shell pkg-config --cflags spice-protocol)

CC = clang
CPPFLAGS += -D_GNU_SOURCE=1 $(INCLUDES)
CFLAGS += -std=c99 -Wall -Wextra -Wshadow -Wpedantic -O3 -Wno-zero-length-array

V ?= 0

LD = clang
LDFLAGS +=
LIBS += \
   $(shell pkg-config --libs glib-2.0) \
   $(shell pkg-config --libs spice-client-glib-2.0) \
   $(shell pkg-config --libs spice-protocol)

HDR = $(wildcard src/*.h)
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=obj/%.o)
MAIN_SRC = $(shell grep -Pl '^int\s+main\s*\x28int\b' $(SRC))
BIN = $(MAIN_SRC:src/%.c=%)

.PHONY: all clean

.SECONDARY: $(OBJ)

all: $(BIN)

$(BIN): $(OBJ)
ifeq ($V, 0)
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
else
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
endif

obj/%.o: src/%.c $(HDR)
	@mkdir -p obj/
ifeq ($V, 0)
	@echo "CC $<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
else
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
endif

clean:
	rm -f $(OBJ) $(BIN)
