#include <qbt/ssa.h>

void ssa(struct fn *f)
{
	/* do a depth-first traversal of blocks, mark locations within as either
	 * generated (i.e. we assign something to the register) or required
	 * (i.e. it must be taken as a block parameter) */
}
