PROGS= ipacutil
MAN8DOCS=doc/ipacutil.8
MAN5DOCS=doc/ipacconf.5
SUBDIRS=

#Set this to your C compiler of choice
CC=/usr/bin/gcc

#Set this to the directory to install the compiled tool
BINDIR= /usr/bin

#Set this to your man directory
MANDIR=/usr/share/man

#MAN8DIR is for documentation for root tools
MAN8DIR=$(MANDIR)/man8

#MAN5DIR is for file formats
MAN5DIR=$(MANDIR)/man5

#####################################################
# This uses a non-portable function on Linux.  Most likely
# it would work without the call on other OSes.  Try it
OS=LINUX
#
# THIS IS CURRENTLY UNUSED
TYPE=USB
# NOTE: PS2 is not supported yet
#TYPE=PS2
#####################################################
all:    $(PROGS) subdirs

ipacutil: ipac_prog.o main.o cfg_util.o cmd_line.o util.o
	$(CC) ipac_prog.o main.o cfg_util.o cmd_line.o util.o -o ipacutil -lusb -lm

ipac_prog.o: ipac_prog.h ipac_prog.c
	$(CC) -c -D$(OS) ipac_prog.c

main.o: main.c
	$(CC) -c main.c

cfg_util.o: cfg_util.c cfg_util.h
	$(CC) -c cfg_util.c

cmd_line.o: cmd_line.c cmd_line.h
	$(CC) -c cmd_line.c

util.o: util.c util.h
	$(CC) -c util.c

subdirs: $(patsubst %, _dir_%, $(SUBDIRS))

$(patsubst %, _dir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

install:        $(PROGS) man_5install man_8install
	install $(PROGS) $(BINDIR)

man_5install: $(MAN5DOCS)
	install $(MAN5DOCS) $(MAN5DIR)

man_8install: $(MAN8DOCS)
	install $(MAN8DOCS) $(MAN8DIR)

clean: subdirs_clean
	-rm -f $(PROGS) *.o *~

subdirs_clean: $(patsubst %, _dir_%clean, $(SUBDIRS))

$(patsubst %, _dir_%clean, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%clean, %, $@) clean
