CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic

PROGRAMS := Tp1/tp1_scanner Tp1/scanner_reels Tp2/tp2_parser

.PHONY: all clean

all: $(PROGRAMS)

Tp1/tp1_scanner: Tp1/tp1_scanner.c
	$(CC) $(CFLAGS) $< -o $@

Tp1/scanner_reels: Tp1/scanner_reels.c
	$(CC) $(CFLAGS) $< -o $@

Tp2/tp2_parser: Tp2/tp2_parser.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS)

