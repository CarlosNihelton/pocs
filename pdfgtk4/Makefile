PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk4 poppler poppler-glib)
LDFLAGS = $(shell $(PKGCONFIG) --libs gtk4 poppler poppler-glib)

pdfgtk4: main.o
	$(CC) $(CFLAGS) -o pdfgtk4 main.o $(LDFLAGS) -fsanitize=address

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -g -O2 -fsanitize=address

.PHONY: deps
deps:
	sudo apt-get install -y libpoppler-glib-dev libgtk-4-dev

