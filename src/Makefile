# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of Linux-ddt.
#
# Linux-ddt is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# Linux-ddt is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Linux-ddt. If not, see <https://www.gnu.org/licenses
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
dispatch.o: dispatch.c $(INCL) term.h ccmd.h user.h debugger.h aeval.h
term.o: term.c
ccmd.o: ccmd.c ccmd.h $(INCL) user.h term.h debugger.h
jobs.o: jobs.c $(INCL) user.h term.h debugger.h
user.o: user.c $(INCL) term.h
files.o: files.c $(INCL) term.h
debugger.o: debugger.c $(INCL)
aeval.o: aeval.c aeval.h jobs.h
