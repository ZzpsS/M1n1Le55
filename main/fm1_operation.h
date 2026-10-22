#pragma once
#include <stdbool.h>

typedef enum {
    FM1_OPERATION_ADAPTER = 1,
    FM1_OPERATION_UPDATE
} fm1_operation_owner_t;

/* Non-blocking, non-recursive service ownership. Acquire before enqueueing and
 * retain through all device I/O and cleanup. This can cross worker tasks, unlike
 * a FreeRTOS mutex. Contending services fail immediately rather than waiting. */
bool fm1_operation_acquire(fm1_operation_owner_t owner);
void fm1_operation_release(fm1_operation_owner_t owner);
