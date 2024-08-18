.PHONY: all
all: check

TESTS :=
include tests.mk

.PHONY: check
check: $(TESTS)
	@cd reports; for d in * ; do				\
		if [ ! -f "$$d/OK" ]; then			\
			echo "BROKEN: $$d" ;			\
		elif [ "$$(tail -n1 $$d/OK)" != "OK" ]; then	\
			echo "FAIL: $$d" ;			\
		fi						\
	done
	@echo "Done."
