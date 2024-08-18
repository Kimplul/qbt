# this could be done better
RELEASE		?= 0
OPTFLAGS	!= [ "$(RELEASE)" != "0" ] \
			&& echo "-O2 -flto=auto" \
			|| echo "-O0"

DEBUG		?= 1
DEBUGFLAGS	!= [ "$(DEBUG)" != "0" ] \
			&& echo "-DDEBUG=1" \
			|| echo "-DNDEBUG=1"

ASSERT		?= 1
ASSERTFLAGS	!= [ "$(ASSERT)" != "0" ] \
			&& echo "-DASSERT=1" \
			|| echo

DEPFLAGS	= -MT $@ -MMD -MP -MF $@.d
LINTFLAGS	:= -fsyntax-only
PREPROCESS	:= -E

LLVM		?= 0
BUILD		:= build

all: qbt

include gen/source.mk

# default values, overwrite if/when needed
CROSS_COMPILE	:=

OBJCOPY		!= [ "$(LLVM)" != "0" ] \
			&& echo llvm-objcopy \
			|| echo $(CROSS_COMPILE)objcopy

COMPILER	!= [ -n "$(CROSS_COMPILE)" ]						\
			&& {								\
				[ "$(LLVM)" != "0" ]					\
					&& echo clang --target="$(CROSS_COMPILE)"	\
					|| echo $(CROSS_COMPILE)gcc			\
					;						\
			}								\
			|| echo $(CC)

OBFLAGS		:= -g
WARNFLAGS	:= -Wall -Wextra

COMPILE_FLAGS	:= $(CFLAGS) $(WARNFLAGS) $(OPTFLAGS) $(OBFLAGS) $(ASSERTFLAGS) \
		   $(DEBUGFLAGS)

INCLUDE_FLAGS	:= -I include

COMPILE		= $(COMPILER) \
		  $(COMPILE_FLAGS) $(DEPFLAGS) $(INCLUDE_FLAGS)

LINT		= $(COMPILER) \
		  $(COMPILE_FLAGS) $(LINTFLAGS) $(INCLUDE_FLAGS)

UBSAN		?= 0
TRISCV_FLAGS	!= [ "$(UBSAN)" != "0" ] \
			&& echo -fsanitize=undefined \
			|| echo

COMPILE_QBT	= $(COMPILE) $(QBT_FLAGS)

-include deps.mk

qbt: $(QBT_OBJS) build/gen/gen_parser.o
	$(COMPILE_QBT) $(QBT_OBJS) build/gen/gen_parser.o -o $@

# might lint some common things twice
.PHONY:
lint: $(TRISCV_LINTS)
