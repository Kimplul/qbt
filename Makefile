DO	!= echo -n > deps.mk

DEBUGFLAGS	!= [ $(RELEASE) ] && echo "-flto=auto -O2 -DNODEBUG" || echo "-O0 -DDEBUG"
CFLAGS		= -Wall -Wextra -g
DEPFLAGS	= -MT $@ -MMD -MP -MF $@.d
LINTFLAGS	= -fsyntax-only
INCLUDEFLAGS	= -Iinclude
COMPILEFLAGS	=
LINKFLAGS	=

all: qbt

# default values
CROSS_COMPILE	?=

# common programs
CC		= gcc

SOURCES		:=

include src/source.mk

COMPILE		= $(CROSS_COMPILE)$(CC) $(DEBUGFLAGS)\
		  $(CFLAGS) $(DEPFLAGS) $(COMPILEFLAGS) $(INCLUDEFLAGS)

LINT		= $(COMPILE) $(LINTFLAGS)

OBJS		!= ./scripts/gen-deps --sources "$(SOURCES)"

include deps.mk

.PHONY: lint
lint: $(OBJS:.o=.o.l)

.PHONY: format
format:
	find src include tests -iname '*.[ch]' |\
		xargs -n 10 -P 0 uncrustify -c uncrustify.conf --no-backup -F -

.PHONY: license
license:
	find src include tests -iname '*.[ch]' |\
		xargs -n 10 -P 0 ./scripts/license

.PHONY: docs
docs:
	find src include -iname '*.[ch]' |\
		xargs ./scripts/warn-undocumented
	doxygen docs/doxygen.conf

.PHONY: check
check: ek
	./tests/check.sh

qbt: $(OBJS)
	$(COMPILE) $(OBJS) -o $@

.PHONY: clean
clean:
	$(RM) -r build qbt deps.mk

.PHONY: clean_docs
clean_docs:
	$(RM) -r docs/output

.PHONY: clean_all
clean_all: clean clean_docs
