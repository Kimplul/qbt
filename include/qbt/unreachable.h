#ifndef UNREACHABLE_H
#define UNREACHABLE_H

#include <qbt/nodes.h>

void remove_unvisited(struct fn *f, int visited);

#endif /* UNREACHABLE_H */
