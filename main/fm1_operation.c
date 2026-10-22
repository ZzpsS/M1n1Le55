#include "fm1_operation.h"
#include <stdatomic.h>

static atomic_uint active_owner;

bool fm1_operation_acquire(fm1_operation_owner_t owner)
{
    if (owner != FM1_OPERATION_ADAPTER && owner != FM1_OPERATION_UPDATE)
        return false;
    unsigned expected = 0;
    return atomic_compare_exchange_strong(&active_owner, &expected, owner);
}

void fm1_operation_release(fm1_operation_owner_t owner)
{
    unsigned expected = owner;
    atomic_compare_exchange_strong(&active_owner, &expected, 0);
}
