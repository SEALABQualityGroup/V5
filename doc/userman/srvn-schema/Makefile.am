# Makefile for bookstore
# Copyright (C) 2005 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# ------------------------------------------------------------------------
# $Id: Makefile.am 11982 2014-04-15 21:32:15Z greg $
# ------------------------------------------------------------------------

SRCS=	srvn-schema.fig

EXTRA_DIST = $(SRCS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
	fig2dev -L eps -m 0.75 $< $@

noinst_DATA = $(SRCS:.fig=.eps) 

distclean-local:
	rm -f $(noinst_DATA)

