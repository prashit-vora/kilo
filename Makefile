CC = cc
CFLAGS = -Wall -Wextra -pedantic -std=c99
TARGET = kilo

all: $(TARGET)

$(TARGET): kilo.c
	$(CC) $(CFLAGS) kilo.c -o $(TARGET)

install: $(TARGET)
	mkdir -p $(HOME)/.local/bin
	cp $(TARGET) $(HOME)/.local/bin/

clean:
	rm -f $(TARGET)
