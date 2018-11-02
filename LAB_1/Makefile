CC = gcc
CFLAGS = -Wall
DEPS = linklayer.h applicationlayer.h
OBJ = main.c linklayer.c applicationlayer.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rcom: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o rcom 

