PREFIX:=/usr

PROGRAMS = clickclack

CFLAGS ?= -O2

all: $(PROGRAMS)

clickclack: clickclack.c
	$(CC) $(CFLAGS) -o clickclack clickclack.c -l SDL2

clean:
	rm -f clickclack

install: $(PROGRAMS)
	install -D -m 0755 clickclack $(DESTDIR)$(PREFIX)/bin/clickclack

