PROGS=ddt
OBJS=main.o dispatch.o term.o ccmd.o jobs.o user.o files.o debugger.o
CFLAGS=-O1 -g

all: $(PROGS)

ddt: $(OBJS)
	$(CC) -o $@ $^

clean:
	$(RM) *.o *~

clobber: clean
	$(RM) $(PROGS)

main.o: main.c term.h dispatch.h jobs.h files.h
dispatch.o: dispatch.c term.h ccmd.h jobs.h user.h files.h debugger.h
term.o: term.c
ccmd.o: ccmd.c ccmd.h jobs.h user.h files.h
jobs.o: jobs.c jobs.h user.h term.h files.h debugger.h
user.o: user.c jobs.h files.h
files.o: files.c files.h jobs.h
debugger.o: debugger.c jobs.h files.h
