PROGS=ddt
OBJS=main.o dispatch.o term.o ccmd.o
CFLAGS=-O1 -g

all: $(PROGS)

ddt: $(OBJS)
	$(CC) -o $@ $^

clean:
	$(RM) *.o *~

clobber: clean
	$(RM) $(PROGS)

main.o: main.c term.h dispatch.h
dispatch.o: dispatch.c term.h ccmd.h
term.o: term.c
ccmd.o: ccmd.c ccmd.h
