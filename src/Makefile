PROGS=ddt
OBJS=main.o dispatch.o term.o ccmd.o jobs.o user.o files.o debugger.o aeval.o
INCL=files.h jobs.h
CFLAGS=-O1 -g

all: $(PROGS)

ddt: $(OBJS)
	$(CC) -o $@ $^

clean:
	$(RM) *.o *~

clobber: clean
	$(RM) $(PROGS)

main.o: main.c $(INCL) term.h dispatch.h
dispatch.o: dispatch.c $(INCL) term.h ccmd.h user.h debugger.h
term.o: term.c
ccmd.o: ccmd.c ccmd.h $(INCL) user.h term.h
jobs.o: jobs.c $(INCL) user.h term.h debugger.h
user.o: user.c $(INCL) term.h
files.o: files.c $(INCL) term.h
debugger.o: debugger.c $(INCL)
aeval.o: aeval.c
