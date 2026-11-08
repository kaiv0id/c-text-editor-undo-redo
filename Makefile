CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2
TARGET = text_editor
SOURCE = src/text_editor.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
