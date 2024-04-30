#include <stdio.h>

#include <qbt/opt.h>
#include <qbt/regalloc.h>
#include <qbt/abi.h>

void optimize(struct fn *f)
{
	printf("\n// initial:\n");
	dump_function(f);

	f->ntmp = correct(f, f->ntmp);
	printf("\n// corrections:\n");
	dump_function(f);

	/* unreachability is done in several steps I guess */
	ssa(f);
	printf("\n// after SSA:\n");
	dump_function(f);

	abi0(f);
	printf("\n// after abi0:\n");
	dump_function(f);

	/* ... do more stuff ... */
	regalloc(f);
	printf("\n// after regalloc:\n");
	dump_function(f);
}
