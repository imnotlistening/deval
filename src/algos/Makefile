#
# Build the algorithm implementations.
#

INCLUDE   = ../../include

CC        = gcc
LD        = ld
CFLAGS    = -Wall -ggdb # -m32
CPPFLAGS  = -D_INFO #-D_DEBUG # Uncomment for internal debug statements.
CPPFLAGS += -I$(INCLUDE)
LIBS      = -lm -lpthread -L.. -ldeval

OBJECTS   = 
PROGS     = root_finder

all: $(OBJECTS) $(PROGS)

.c.o:
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(OBJECTS) $(PROGS)