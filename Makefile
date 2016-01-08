CC=gcc
LD=gcc

CFLAGS=-O3 -g -Wall
LDFLAGS=


OBJS=psf.o main.o
OUT=psf-dump


$(OUT): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm $(OBJS) $(OUT)
