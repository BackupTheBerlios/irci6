CC = gcc
LD = $(CC)
DEBUG = -DIRC6_DEBUG
CFLAGS = -g -O2 -Wall -W -Wpointer-arith -Wno-unused \
	 -I.  -Imissing -Ilib -Iautogen -pipe $(DEBUG)
LDFLAGS = -Lmissing
LIBS = -levent 

C6SRC = autogen/c6/marshal.c autogen/c6/userinfo_struct.c 		\
	autogen/c6/userinfo_hash.c					\
	c6/server.c c6/client.c c6/c6.c c6/codifica.c c6/userinfo.c
IRCSRC = autogen/irc/marshal.c autogen/irc/cmd_list.c 			\
	 autogen/irc/cmd_console.c autogen/irc/cmd_searchusr.c 		\
	 irc/irc.c irc/room.c irc/status.c irc/output.c irc/dump.c
MISSINGSRC = missing/strlcat.c missing/strlcpy.c missing/strcasecmp.c	\
	     missing/md5.c missing/blowfish.c
LIBSRC = lib/area.c lib/area_chunk.c lib/args_sep.c lib/buffer.c 	\
	 lib/hash.c lib/signals.c

SRCS = main.c socket.c list.c $(C6SRC) $(IRCSRC) $(LIBSRC) $(MISSINGSRC)
OBJS = $(SRCS:.c=.o)

AGINC = autogen/irc/func.h autogen/c6/func.h autogen/version.h
IRCINC = irc/comandi.c irc/console.c irc/searchusr.c

TOOLS = tool/mkmarshal tool/mkptypes
AG_DIR = autogen/c6 autogen/irc

.SUFFIXES: .c .o .gperf

all: $(AG_DIR) irC6

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

irC6: $(TOOLS) $(SRCS) $(AGINC) $(OBJS) 
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

install: irC6
	-install -c -m 0755 -g bin irC6 /usr/local/bin/

uninstall:
	-rm -f /usr/local/bin/irC6

clean:
	-rm -fr .depend.bak $(OBJS) irC6

# dev

nodev: Makefile
	sed 	-e'/# dev/q' \
		-e's/DEBUG.*=.*/DEBUG =/' \
		-e's/TOOLS.*=.*/TOOLS =/' \
			Makefile > Makefile.nodev

src: $(TOOLS) $(AG_DIR) $(SRCS) $(AGINC)

autogen/c6:
	mkdir autogen/c6

autogen/irc:
	mkdir autogen/irc

autogen/version.h: ../VERSION
	awk '{print "#define VERSION \""$$1"\""}' ../VERSION > $@

autogen/irc/func.h: tool/mkptypes $(IRCSRC) $(IRCINC)
	tool/mkptypes -Ax $(IRCSRC) $(IRCINC) > $@

autogen/c6/func.h: tool/mkptypes $(C6SRC)
	tool/mkptypes -Ax $(C6SRC) > $@

autogen/c6/marshal.c autogen/c6/marshal.h: c6/marshal.in tool/mkmarshal
	tool/mkmarshal -oautogen/c6/marshal -iirC6.h -pc6msl_ c6/marshal.in

autogen/irc/marshal.c autogen/irc/marshal.h: irc/marshal.in tool/mkmarshal
	tool/mkmarshal -oautogen/irc/marshal -iirC6.h -pircmsl_ irc/marshal.in

autogen/irc/reply.h: irc/reply
	awk -f tool/make_comment.awk ../COPYRIGHT > $@
	awk -f irc/make_reply.awk irc/reply >> $@

autogen/c6/userinfo_struct.c: c6/profilo.ini c6/profilo_nomi.ini
	awk -f tool/make_comment.awk ../COPYRIGHT > $@
	cat c6/profilo.ini c6/profilo_nomi.ini | \
		awk -f c6/make_userinfo.awk >> $@

autogen/c6/userinfo_hash.gperf: c6/profilo.ini c6/profilo_nomi.ini
	echo '%{' > $@
	awk -f tool/make_comment.awk ../COPYRIGHT >> $@
	cat c6/profilo.ini c6/profilo_nomi.ini | \
		awk -f c6/make_uihash.awk >> $@

autogen/irc/cmd_list.c: irc/cmd_list.gperf irc/comandi.c
	tool/ag_gperf.sh irc/cmd_list
autogen/irc/cmd_console.c: irc/cmd_console.gperf irc/console.c
	tool/ag_gperf.sh irc/cmd_console
autogen/irc/cmd_searchusr.c: irc/cmd_searchusr.gperf irc/searchusr.c
	tool/ag_gperf.sh irc/cmd_searchusr
autogen/c6/userinfo_hash.c: autogen/c6/userinfo_hash.gperf
	tool/ag_gperf.sh autogen/c6/userinfo_hash
$(IRCSRC): autogen/irc/reply.h


.gperf.c:
	gperf -t -L KR-C -N $(*:irc/%=%) -D -G -C \
		--ignore-case -W wordlist -o $< > $@ 

distclean:
	make clean
	-rm -fr $(TOOLS)

cleanag: 
	make distclean
	-rm -fr autogen/c6 autogen/irc autogen/*.[ch]

dep: $(SRCS)
	echo > .depend
	makedepend -f.depend -- $(CFLAGS) -- $(SRCS)

