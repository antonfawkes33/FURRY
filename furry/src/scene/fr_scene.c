#include "fr_scene.h"
#include "../core/fr_log.h"
#include <stdlib.h>
#include <string.h>

FrNode *fr_node_create(const char *name) {
  FrNode *node = (FrNode *)malloc(sizeof(FrNode));
  if (!node) {
    FR_ERROR("Failed to allocate FrNode");
    return NULL;
  }
  memset(node, 0, sizeof(FrNode));

  if (name) {
    strncpy(node->name, name, FR_NODE_NAME_MAX - 1);
    node->name[FR_NODE_NAME_MAX - 1] = '\0';
  }

  node->scale = (FrVec2){1.0f, 1.0f};
  node->world_transform = fr_mat4_identity();
  return node;
}

void fr_node_destroy(FrNode *node) {
  if (!node)
    return;
  // Detach from parent without destroying anything
  fr_node_remove_from_parent(node);
  free(node);
}

void fr_node_set_parent(FrNode *child, FrNode *parent) {
  if (!child)
    return;

  // Remove from old parent first
  if (child->parent)
    fr_node_remove_from_parent(child);

  child->parent = parent;

  if (parent) {
    if (parent->child_count < FR_NODE_MAX_CHILDREN) {
      parent->children[parent->child_count++] = child;
    } else {
      FR_ERROR("FrNode '%s': maximum child count (%d) reached on '%s'",
               child->name, FR_NODE_MAX_CHILDREN, parent->name);
    }
  }
}

void fr_node_remove_from_parent(FrNode *child) {
  if (!child || !child->parent)
    return;

  FrNode *parent = child->parent;
  for (uint32_t i = 0; i < parent->child_count; i++) {
    if (parent->children[i] == child) {
      // Swap with last element to avoid gaps
      parent->children[i] = parent->children[--parent->child_count];
      parent->children[parent->child_count] = NULL;
      break;
    }
  }
  child->parent = NULL;
}

void fr_node_set_position(FrNode *node, FrVec2 pos) {
  if (node)
    node->position = pos;
}

void fr_node_set_scale(FrNode *node, FrVec2 scale) {
  if (node)
    node->scale = scale;
}

void fr_node_set_rotation(FrNode *node, float angle_rad) {
  if (node)
    node->rotation = angle_rad;
}

void fr_node_update_transform(FrNode *node) {
  if (!node)
    return;

  // Local TRS = T * R * S
  FrMat4 T = fr_mat4_translate(node->position.x, node->position.y);
  FrMat4 R = fr_mat4_rotate_z(node->rotation);
  FrMat4 S = fr_mat4_scale(node->scale.x, node->scale.y);
  FrMat4 local = fr_mat4_mul(T, fr_mat4_mul(R, S));

  if (node->parent) {
    node->world_transform = fr_mat4_mul(node->parent->world_transform, local);
  } else {
    node->world_transform = local;
  }

  // Recurse over children
  for (uint32_t i = 0; i < node->child_count; i++) {
    fr_node_update_transform(node->children[i]);
  }
}

FrVec2 fr_node_get_world_position(const FrNode *node) {
  if (!node)
    return (FrVec2){0.0f, 0.0f};
  // Translation is stored in column 3 of the column-major matrix
  return (FrVec2){node->world_transform.m[12], node->world_transform.m[13]};
}

FrVec2 fr_node_get_world_scale(const FrNode *node) {
  if (!node)
    return (FrVec2){1.0f, 1.0f};
  // Scale is the length of the basis vectors (columns 0 and 1)
  const float *m = node->world_transform.m;
  float sx = sqrtf(m[0] * m[0] + m[1] * m[1]);
  float sy = sqrtf(m[4] * m[4] + m[5] * m[5]);
  return (FrVec2){sx, sy};
}
