#
# Makefile for CTE tape reading program
#
# $Id: Makefile,v 1.2 1994/12/10 20:28:33 jak Exp $
#
# History:
# $Log: Makefile,v $
# Revision 1.2  1994/12/10 20:28:33  jak
# Fixed a bug in the channel code.  -jak
#
#

CC=cc
CFLAGS=-ggdb3 -O
LIBS=

all: cte

cte: cte.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
	
install: cte
	cp cte $(HOME)/Apps



