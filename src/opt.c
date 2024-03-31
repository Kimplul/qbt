#include <qbt/opt.h>
#include <qbt/regalloc.h>
#include <qbt/ssa.h>
#include <qbt/abi.h>

void optimize(struct fn *f)
{
	ssa(f);
	abi0(f);
	/* ... do more stuff ... */
	regalloc(f);
}
