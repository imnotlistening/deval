#
# Build libdevol.so. Simple Makefile, everything is pretty self explanitory.
#

SHELL=/bin/bash

# Get the $VERSION
include ../config.mak

CC        = gcc
LD        = gcc # Transaltes to the SunOS linker for us.
CFLAGS    = -Wall -ggdb # -m32
CPPFLAGS  = #-D_INFO -D_DEBUG # Uncomment for internal debug statements.
CPPFLAGS += -I../include
LDFLAGS   = -shared # -melf_i386 
LIBS      = -lm -lpthread

OBJECTS   = devol.o devol_threads.o util.o
TESTS     = thread_test devol_test data_sizes
INCLUDE   = ../include
HEADERS   = $(INCLUDE)/client.h

all: libdeval $(TESTS) examples

#
# Special targets.
#
libdeval: $(OBJECTS)
	$(LD) $(LDFLAGS) -o libdeval.so.$(REVISION) $(OBJECTS) $(LIBS)
	if [ `uname -s` != "SunOS" ]; then \
	ldconfig -v -n . ; \
	else  \
	ln -s -f libdeval.so.$(REVISION) libdeval.so ; \
	fi

#
# This is used for the rest of the C code.
#
.c.o: $(HEADERS)
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.c: libdeval.so.$(REVISION)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS) -L. -ldeval

#
# This is used to build the algorithm implementations.
#
.PHONY: examples
examples: libdeval.so.$(REVISION)
	cd algos && make

#
# Cleaning targets.
#
.PHONY: clean dist-clean
clean:
	rm -f libdeval.so* $(OBJECTS) $(TESTS)
	cd algos && make clean

dist-clean: clean
	rm -f config.mak
