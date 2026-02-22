#ifndef FR_MEMORY_H
#define FR_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// --- Arena Allocator ---
// Used for linear, short-lived allocations (e.g., per-frame or per-scene)
typedef struct FrArena {
    uint8_t *base;
    size_t   size;
    size_t   offset;
} FrArena;

bool    fr_arena_init(FrArena *arena, size_t size);
void    fr_arena_destroy(FrArena *arena);
void*   fr_arena_alloc(FrArena *arena, size_t size);
void    fr_arena_reset(FrArena *arena); // O(1) - just reset offset

// --- Pool Allocator ---
// Used for fixed-size object allocations (e.g., scene nodes, tweens)
typedef struct FrPool {
    uint8_t  *base;
    size_t    slot_size;
    uint32_t  slot_count;
    uint32_t *free_list;
    uint32_t  free_top;
} FrPool;

bool    fr_pool_init(FrPool *pool, size_t slot_size, uint32_t slot_count);
void    fr_pool_destroy(FrPool *pool);
void*   fr_pool_alloc(FrPool *pool);
void    fr_pool_free(FrPool *pool, void *ptr);

#endif // FR_MEMORY_H
