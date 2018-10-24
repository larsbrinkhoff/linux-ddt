PROGS=ddt
OBJS=main.o dispatch.o term.o ccmd.o jobs.o user.o
CFLAGS=-O1 -g

all: $(PROGS)

ddt: $(OBJS)
	$(CC) -o $@ $^

clean:
	$(RM) *.o *~

clobber: clean
	$(RM) $(PROGS)

main.o: main.c term.h dispatch.h
dispatch.o: dispatch.c term.h ccmd.h jobs.h user.h
term.o: term.c
ccmd.o: ccmd.c ccmd.h jobs.h user.h
jobs.o: jobs.c jobs.h user.h
user.o: user.c user.h
