CC = gcc
CFLAGS = -Wall
DEPS = linklayer.h
OBJ = main.c linklayer.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rcom: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o rcom 

