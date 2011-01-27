#
# Build the algorithm implementations.
#

INCLUDE   = -I../../include -I.


CC        = gcc
LD        = ld
CFLAGS    = -Wall -ggdb # -m32
CPPFLAGS  = -D_INFO #-D_DEBUG # Uncomment for internal debug statements.
CPPFLAGS += $(INCLUDE)
LIBS      = -lm -lpthread -L.. -ldeval

OBJECTS   = mixture_fread.o
PROGS     = root_finder mixture

all: $(OBJECTS) $(PROGS)

.c.o:
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS)

mixture: mixture.c mixture_fread.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< mixture_fread.o $(LIBS)

clean:
	rm -f $(OBJECTS) $(PROGS)