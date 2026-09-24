# winlock Makefile
# X11 window control utility

VERSION = 1.0.0

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\"
LDFLAGS =

# Package dependencies
PKG_DEPS = x11
PKG_CFLAGS = $(shell pkg-config --cflags $(PKG_DEPS))
PKG_LIBS = $(shell pkg-config --libs $(PKG_DEPS))

# Source files
SRCDIR = src
BUILDDIR = build

SOURCES = $(SRCDIR)/main.c $(SRCDIR)/config.c $(SRCDIR)/xutil.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

TARGET = $(BUILDDIR)/winlock

.PHONY: all clean install check-deps

all: check-deps $(TARGET)

check-deps:
	@pkg-config --exists $(PKG_DEPS) || (echo "Missing dependencies. Install with:"; \
		echo "  sudo apt install libx11-dev"; \
		exit 1)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Compile source files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -c $< -o $@

# Link
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(PKG_LIBS) -o $@
	@echo "Build complete: $(TARGET)"

clean:
	rm -rf $(BUILDDIR)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/winlock
