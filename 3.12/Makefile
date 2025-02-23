########################################################################
#              _            _   
#   _   _ _ __| | __ _  ___| |_ 
#  | | | | '__| |/ _` |/ _ \ __|
#  | |_| | |  | | (_| |  __/ |_ 
#   \__,_|_|  |_|\__, |\___|\__| - Gets your URL!
#                |___/          
########################################################################

OBJS=urlget.o main.o hugehelp.o
TARGET=urlget

# Linux:
CC = gcc
CFLAGS = -c -Wall -pedantic
CPPFLAGS = -DHAVE_STRCASECMP
LDFLAGS =

# Solaris 2:
#LDFLAGS = -lnsl -lsocket

.c.o: urlget.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $*.c

$(TARGET): $(OBJS) Makefile
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f *.o *~ $(TARGET) hugehelp.c

tgz:
	@(dir=`pwd`;name=`basename $$dir`;echo Creates $$name.tar.gz; cd .. ; \
	tar -cf $$name.tar `cat $$name/FILES | sed "s:^:$$name/:g"` ; \
	gzip $$name.tar ; chmod a+r $$name.tar.gz ; mv $$name.tar.gz $$name/)

zip:
	@(dir=`pwd`;name=`basename $$dir`;echo Creates $$name.zip; cd .. ; \
	zip $$name.zip `cat $$name/FILES | sed "s:^:$$name/:g"` ; \
	chmod a+r $$name.zip ; mv $$name.zip $$name/)

main.o: main.c urlget.h

# This generates the hugehelp.c file
hugehelp.c: README mkhelp
	@rm -f hugehelp.c
	sh mkhelp > hugehelp.c
	@chmod a-w hugehelp.c
