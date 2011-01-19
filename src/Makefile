#
# Build libdevol.so. Simple Makefile, everything is pretty self explanitory.
#

# Get the $VERSION
sinclude ../config.mak

CC        = gcc
LD        = ld
CFLAGS    = -Wall -ggdb # -m32
CPPFLAGS  = -D_INFO #-D_DEBUG # Uncomment for internal debug statements.
CPPFLAGS += -I../include
LDFLAGS   = -shared -soname libdeval.so # -melf_i386 
LIBS      = -lm -lpthread

OBJECTS   = devol.o devol_threads.o util.o
TESTS     = thread_test devol_test data_sizes
INCLUDE   = ../include
HEADERS   = $(INCLUDE)/client.h

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
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.c: libdeval.so.$(REVISION)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS) -L. -ldeval

#
# Cleaning targets.
#
clean:
	rm -f libdeval.so* $(OBJECTS) $(TESTS)

dist-clean: clean
	rm -f config.mak
