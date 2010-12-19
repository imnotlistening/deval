#
# Build my RIT Shell (RSH).
#

# Get the $VERSION
sinclude ../config.mak

CC       = gcc
LD       = ld
CFLAGS   = -Wall -ggdb 
CPPFLAGS = -I../include
LDFLAGS  = -shared -soname libdeval.so
LIBS     = -lm

OBJECTS  = devol.c
TESTS    =
INCLUDE  = ../include
HEADERS  = $(INCLUDE)/client.h

all: libdeval $(TESTS)

#
# Special targets.
#
libdeval: $(OBJECTS)
	$(LD) $(LDFLAGS) -o libdeval.so.$(REVISION) $(OBJECTS) $(LIBS)
	ldconfig -v -n .

#
# This is used for the rest of the C code.
#
.c.o: $(HEADERS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.c: libdeval.so.$(REVISION)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS) -L. -ldeval

#
# Cleaning targets.
#
clean:
	rm -f libdeval.so* $(OBJECTS) $(TESTS)

dist-clean: clean
	rm -f config.mak
