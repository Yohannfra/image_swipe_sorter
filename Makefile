CC = gcc
CFLAGS = -Wall -Wextra -O2 -Isrc
LDFLAGS = -lSDL2 -lSDL2_image

TARGET = image_swipe_sorter
SRCDIR = src
OBJDIR = obj

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: all clean format lint re

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(TARGET) $(OBJDIR)

format:
	clang-format -i $(SRCDIR)/*.c $(SRCDIR)/*.h

lint:
	cppcheck --enable=all --suppress=missingIncludeSystem --suppress=constParameter $(SRCDIR)

re: clean all
