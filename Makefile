.PHONY: all
all: setup
	$(MAKE) -f scripts/makefile

# this kicks all unrecognised targets to the client script.
# note that trying to compile individual files, e.g.
#
#	make kernel.elf
#
# will not work, you would need
#
#	make -f scripts/makefile kernel.elf
#
# instead
.DEFAULT: setup
	$(MAKE) -f scripts/makefile $<

.PHONY: analyze
analyze: setup
	CC='gcc -fanalyzer' SKIP_ANALYZER='-fno-analyzer' $(MAKE) CROSS_COMPILE=

.PHONY: check
check: all
	$(MAKE) -C tests -k check

.PHONY:
setup:
	@echo -n > deps.mk
	@./scripts/gen-deps -p QBT -c COMPILE_QBT -b qbt "$(QBT_SOURCES)"

CLEANUP		:= build deps.mk qbt
CLEANUP_CMD	:=
QBT_SOURCES	:=

include src/source.mk

.PHONY: format
format:
	find src include -iname '*.[ch]' |\
		xargs uncrustify -c uncrustify.conf --no-backup -F -

.PHONY: license
license:
	find src include -iname '*.[ch]' |\
		xargs ./scripts/license

.PHONY: docs
docs:
	find src include -iname '*.[ch]' -not -path */gen/* |\
		xargs ./scripts/warn-undocumented
	doxygen docs/doxygen.conf

RM	= rm

.PHONY: clean
clean:
	$(RM) -rf $(CLEANUP)

.PHONY: clean_docs
clean_docs:
	$(RM) -rf docs/output

.PHONY: clean_all
clean_all: clean clean_docs
