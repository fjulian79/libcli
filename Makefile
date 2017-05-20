BINARY      = libcli
COMPATIBLE  = cortex-m3
DEPENDS     = 
SRCEXT      ?= cpp
DEFS        =
INCLUDE     = $(SRCDIR)/include
OPT         = -Og
EXPORT      = include/cli.h

###############################################################################
# Call generic library makefile
###############################################################################
include $(MYBUILDROOTPATH)/makefiles/library.mk
