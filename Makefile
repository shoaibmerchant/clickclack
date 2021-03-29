PREFIX:=/usr

PROGRAMS = clickclack

all: $(PROGRAMS)

clickclack: clickclack.c
	gcc -o clickclack clickclack.c -l SDL2

clean:
	rm -f clickclack

install: $(PROGRAMS)
	install -D -m 0755 clickclack $(DESTDIR)$(PREFIX)/bin/clickclack

