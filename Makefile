#
# Makefile for CTE tape reading program
#
# $Id: Makefile,v 1.1.1.1 1994/12/06 05:23:39 jak Exp $
#
# History:
# $Log: Makefile,v $
# Revision 1.1.1.1  1994/12/06 05:23:39  jak
# Initial import of CTE_Extractor code.   -jak
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



