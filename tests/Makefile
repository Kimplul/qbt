check:
	$(MAKE) -f scripts/makefile check

.DEFAULT:
	$(MAKE) -f scripts/makefile $<

SOURCES	!= echo */source.mk
DO	!= echo -n > tests.mk && rm -rf reports

SIMPLE :=
SIMPLE_XFAIL :=

include $(SOURCES)

# pass through xargs to 'trick' it into expanding each SIMPLE_XFAIL into a
# separate argument for the script, as make would otherwise just split at each space
# this way we can pass spaces as arguments to the generator scripts, pretty neat
# huh?
DO	!= echo "$(SIMPLE)" | xargs ./scripts/gen-simple
DO	!= echo "$(SIMPLE_XFAIL)" | xargs ./scripts/gen-simple-xfail

RM	?= rm

.PHONY: clean
clean:
	$(RM) -rf reports
