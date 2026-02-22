#ifndef FR_SCENE_H
#define FR_SCENE_H

#include "../core/fr_math.h"
#include <stdint.h>

#define FR_NODE_MAX_CHILDREN 32
#define FR_NODE_NAME_MAX 64

// A scene node with a local TRS transform and a computed world matrix.
// Call fr_node_update_transform on the root each frame before drawing.
typedef struct FrNode {
  char name[FR_NODE_NAME_MAX];

  // Local transform components
  FrVec2 position;
  FrVec2 scale;
  float rotation; // radians, counter-clockwise

  // Computed world transform (updated by fr_node_update_transform)
  FrMat4 world_transform;

  // Hierarchy
  struct FrNode *parent;
  struct FrNode *children[FR_NODE_MAX_CHILDREN];
  uint32_t child_count;
} FrNode;

// --- Lifecycle ---
FrNode *fr_node_create(const char *name);
void fr_node_destroy(FrNode *node); // does NOT destroy children

// --- Hierarchy ---
// Sets child's parent and registers child in parent's children array.
void fr_node_set_parent(FrNode *child, FrNode *parent);
// Removes child from parent's children array (does not destroy either).
void fr_node_remove_from_parent(FrNode *child);

// --- Local transform setters ---
void fr_node_set_position(FrNode *node, FrVec2 pos);
void fr_node_set_scale(FrNode *node, FrVec2 scale);
void fr_node_set_rotation(FrNode *node, float angle_rad);

// --- Transform update ---
// Recomputes world_transform for this node and recurses over all children.
// Call on the root node once per frame before issuing draw calls.
void fr_node_update_transform(FrNode *node);

// --- World-space accessors ---
// Derived from world_transform — valid only after fr_node_update_transform.
FrVec2 fr_node_get_world_position(const FrNode *node);
FrVec2 fr_node_get_world_scale(const FrNode *node);

#endif // FR_SCENE_H
