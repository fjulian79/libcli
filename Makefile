BINARY      = libcli
COMPATIBLE  = cortex-m3
DEPENDS     = 
SRCEXT      ?= cpp
DEFS        =
WARN_FLAGS  = -Wall
INCLUDE     = 
OPT         = -Og
EXPORT      = cli.h

###############################################################################
# Call generic library makefile
###############################################################################
include $(MYBUILDROOTPATH)/makefiles/library.mk
