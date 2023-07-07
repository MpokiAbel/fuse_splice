CC = gcc -g
CFLAGS = -Wall -Wextra
FUSEINCLUDE = `pkg-config fuse3 --cflags --libs`
SRCDIR = src
BUILDDIR = build

# List of source files
SOURCES = $(wildcard $(SRCDIR)/*.c)

# List of object files
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))

# Name of the executable
TARGET = $(BUILDDIR)/main

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(FUSEINCLUDE) -lcurl -o $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(FUSEINCLUDE) -c $< -o $@;

test:
	./$(TARGET) -d mount/

clean:
	rm -f $(OBJECTS) $(TARGET)

unmount:
	fusermount3 -uz mount