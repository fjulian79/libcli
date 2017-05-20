BINARY      = libcli
COMPATIBLE  = cortex-m3
DEPENDS     = 
SOURCES     = 
DEFS        =
INCLUDE     = $(SRCDIR)/include
OPT         = -Og
EXPORT      = include/cli.h

###############################################################################
# Call generic library makefile
###############################################################################
include $(MYBUILDROOTPATH)/makefiles/library.mk
