CC = gcc
CFLAGS =
LDFLAGS =
VERS = `cat VERSION`

all: .gcc_test.c .event_test.c 
	@cd src; make -f Makefile.nodev

.gcc_test.c:
	./gcc_test.sh $(CC) $(CFLAGS) $(LDFLAGS)

.event_test.c:
	./ev_test.sh $(CC) $(CFLAGS) $(LDFLAGS)

install:
	@cd src; make -f Makefile.nodev all install

uninstall:
	@cd src; make -f Makefile.nodev uninstall

clean:
	@cd src; make clean
	-rm -f .event_test.* .gcc_test.*
	-rm -fr irC6-*
	-find . \( -name '*~' -o -name 'core' -o -name '*.core' \) \
		-exec rm -f '{}' ';'

pkt: clean
	@cd src; make src distclean
	cp -r . ../irC6.tmp
	-find ../irC6.tmp -name CVS -type d -exec rm -fr '{}' ';'
	mv ../irC6.tmp irC6-$(VERS)
	tar cf ../irC6-$(VERS).tar irC6-$(VERS)
	make clean

static_pkt: clean
	mkdir irC6-$(VERS)
	cd src; make -f Makefile.nodev LDFLAGS='-L. -static'
	cp AUTHOR README LEGGIMI TODO COPYRIGHT VERSION src/irC6 irC6-$(VERS)
	tar cf ../irC6-$(VERS)_static_`uname`_`uname -m`.tar irC6-$(VERS)
	make clean

deb: debian/control
	dpkg-buildpackage -b
	debian/rules clean
