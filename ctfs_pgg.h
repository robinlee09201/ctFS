#include "ctfs_format.h"
#include "ctfs_util.h"
#include "ctfs_runtime.h"

pgg_level_t pgg_get_lvl(uint64_t size);

relptr_t pgg_allocate(pgg_level_t level);

void pgg_deallocate(pgg_level_t level, relptr_t target);

relptr_t pgg_mkfs();

extern const uint64_t pgg_limit[10];

extern const uint64_t pgg_size[10];
