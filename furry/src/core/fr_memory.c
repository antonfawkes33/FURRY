#include "fr_memory.h"
#include <stdlib.h>
#include <string.h>

// --- Arena ---

bool fr_arena_init(FrArena *arena, size_t size) {
  arena->base = (uint8_t *)malloc(size);
  if (!arena->base)
    return false;
  arena->size = size;
  arena->offset = 0;
  return true;
}

void fr_arena_destroy(FrArena *arena) {
  if (arena->base) {
    free(arena->base);
    arena->base = NULL;
  }
  arena->size = 0;
  arena->offset = 0;
}

void *fr_arena_alloc(FrArena *arena, size_t size) {
  // Aligining to 8 bytes for safety
  size = (size + 7) & ~7;

  if (arena->offset + size > arena->size) {
    return NULL; // Out of memory
  }

  void *ptr = arena->base + arena->offset;
  arena->offset += size;
  return ptr;
}

void fr_arena_reset(FrArena *arena) { arena->offset = 0; }

// --- Pool ---

bool fr_pool_init(FrPool *pool, size_t slot_size, uint32_t slot_count) {
  // Ensure slot size is at least pointer-sized and aligned
  slot_size = (slot_size + 7) & ~7;
  if (slot_size < sizeof(void *))
    slot_size = sizeof(void *);

  pool->base = (uint8_t *)malloc(slot_size * slot_count);
  if (!pool->base)
    return false;

  pool->free_list = (uint32_t *)malloc(sizeof(uint32_t) * slot_count);
  if (!pool->free_list) {
    free(pool->base);
    pool->base = NULL;
    return false;
  }

  pool->slot_size = slot_size;
  pool->slot_count = slot_count;
  pool->free_top = slot_count;

  // Fill free list in reverse so first alloc is at start of base
  for (uint32_t i = 0; i < slot_count; ++i) {
    pool->free_list[i] = slot_count - 1 - i;
  }

  return true;
}

void fr_pool_destroy(FrPool *pool) {
  if (pool->base)
    free(pool->base);
  if (pool->free_list)
    free(pool->free_list);
  memset(pool, 0, sizeof(FrPool));
}

void *fr_pool_alloc(FrPool *pool) {
  if (pool->free_top == 0)
    return NULL;

  uint32_t index = pool->free_list[--pool->free_top];
  return pool->base + (index * pool->slot_size);
}

void fr_pool_free(FrPool *pool, void *ptr) {
  if (!ptr)
    return;

  uint8_t *p = (uint8_t *)ptr;
  if (p < pool->base ||
      p >= pool->base + (pool->slot_size * pool->slot_count)) {
    return; // Pointer out of bounds
  }

  size_t offset = p - pool->base;
  uint32_t index = (uint32_t)(offset / pool->slot_size);

  if (pool->free_top < pool->slot_count) {
    pool->free_list[pool->free_top++] = index;
  }
}
