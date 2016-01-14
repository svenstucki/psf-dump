CC=gcc
LD=gcc

CFLAGS=-O3 -g -Wall
LDFLAGS=-lz


OBJS=psf.o main.o
OUT=psf-dump


$(OUT): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c psf.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm $(OBJS) $(OUT)
