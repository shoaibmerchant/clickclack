PREFIX:=/usr

PROGRAMS = clickclack

all: $(PROGRAMS)

clickclack: clickclack.c
	gcc -g -o clickclack clickclack.c

clean:
	rm -f clickclack

install: $(PROGRAMS)
	install -D -m 0755 clickclack $(DESTDIR)$(PREFIX)/bin/

