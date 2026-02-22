#ifndef FR_MATH_H
#define FR_MATH_H

#include <math.h>
#include <string.h>

typedef struct FrVec2 {
  float x, y;
} FrVec2;

typedef struct FrVec3 {
  float x, y, z;
} FrVec3;

typedef struct FrVec4 {
  float x, y, z, w;
} FrVec4;

typedef struct FrMat4 {
  float m[16]; // Column-major
} FrMat4;

// --- Vectors ---

static inline FrVec2 fr_vec2(float x, float y) { return (FrVec2){x, y}; }
static inline FrVec3 fr_vec3(float x, float y, float z) {
  return (FrVec3){x, y, z};
}
static inline FrVec4 fr_vec4(float x, float y, float z, float w) {
  return (FrVec4){x, y, z, w};
}

static inline FrVec2 fr_vec2_add(FrVec2 a, FrVec2 b) {
  return (FrVec2){a.x + b.x, a.y + b.y};
}
static inline FrVec3 fr_vec3_add(FrVec3 a, FrVec3 b) {
  return (FrVec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline float fr_vec2_length(FrVec2 v) {
  return sqrtf(v.x * v.x + v.y * v.y);
}
static inline FrVec2 fr_vec2_normalize(FrVec2 v) {
  float len = fr_vec2_length(v);
  if (len == 0)
    return v;
  return (FrVec2){v.x / len, v.y / len};
}

// --- Matrices ---

static inline FrMat4 fr_mat4_identity(void) {
  FrMat4 mat = {0};
  mat.m[0] = 1.0f;
  mat.m[5] = 1.0f;
  mat.m[10] = 1.0f;
  mat.m[15] = 1.0f;
  return mat;
}

static inline FrMat4 fr_mat4_ortho(float left, float right, float bottom,
                                   float top, float near_val, float far_val) {
  FrMat4 mat = fr_mat4_identity();
  mat.m[0] = 2.0f / (right - left);
  mat.m[5] = 2.0f / (top - bottom);
  mat.m[10] = -2.0f / (far_val - near_val);
  mat.m[12] = -(right + left) / (right - left);
  mat.m[13] = -(top + bottom) / (top - bottom);
  mat.m[14] = -(far_val + near_val) / (far_val - near_val);
  return mat;
}

static inline FrMat4 fr_mat4_mul(FrMat4 a, FrMat4 b) {
  FrMat4 r = {0};
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      for (int k = 0; k < 4; k++) {
        r.m[col * 4 + row] += a.m[k * 4 + row] * b.m[col * 4 + k];
      }
    }
  }
  return r;
}

static inline FrMat4 fr_mat4_translate(float tx, float ty) {
  FrMat4 mat = fr_mat4_identity();
  mat.m[12] = tx;
  mat.m[13] = ty;
  return mat;
}

static inline FrMat4 fr_mat4_scale(float sx, float sy) {
  FrMat4 mat = fr_mat4_identity();
  mat.m[0] = sx;
  mat.m[5] = sy;
  return mat;
}

static inline FrMat4 fr_mat4_rotate_z(float angle_rad) {
  FrMat4 mat = fr_mat4_identity();
  float c = cosf(angle_rad);
  float s = sinf(angle_rad);
  mat.m[0] = c;
  mat.m[4] = -s;
  mat.m[1] = s;
  mat.m[5] = c;
  return mat;
}

// Apply a mat4 to a 2D point (w=1, z=0).
static inline FrVec2 fr_mat4_transform_vec2(FrMat4 m, FrVec2 v) {
  float x = m.m[0] * v.x + m.m[4] * v.y + m.m[12];
  float y = m.m[1] * v.x + m.m[5] * v.y + m.m[13];
  return (FrVec2){x, y};
}

#endif // FR_MATH_H
