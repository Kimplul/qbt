#ifndef OPT_H
#define OPT_H

#include <qbt/nodes.h>
void optimize(struct fn *fn);
size_t ssa(struct fn *f);
size_t correct(struct fn *f, size_t ri);

#endif /* OPT_H */
