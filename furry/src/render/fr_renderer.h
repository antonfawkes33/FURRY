#ifndef FR_RENDER_H
#define FR_RENDER_H

#include "../core/fr_math.h"
#include "../platform/fr_platform.h"
#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

typedef struct FrVertex {
  FrVec2 pos;
  FrVec4 color;
  FrVec2 uv;
} FrVertex;

typedef struct FrRenderer FrRenderer;
typedef struct FrTexture FrTexture;

typedef struct FrRendererInfo {
  const char *app_name;
  uint32_t app_version;
} FrRendererInfo;

FrRenderer *fr_renderer_init(FrWindow *window, const FrRendererInfo *info);
void fr_renderer_shutdown(FrRenderer *renderer);

// Buffers
bool fr_renderer_create_buffer(FrRenderer *renderer, VkDeviceSize size,
                               VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties,
                               VkBuffer *buffer, VkDeviceMemory *buffer_memory);
void fr_renderer_destroy_buffer(FrRenderer *renderer, VkBuffer buffer,
                                VkDeviceMemory memory);
void fr_renderer_upload_to_buffer(FrRenderer *renderer, VkDeviceMemory memory,
                                  const void *data, VkDeviceSize size);

// Shaders
VkShaderModule fr_renderer_create_shader_module(FrRenderer *renderer,
                                                const uint32_t *code,
                                                size_t size);
void fr_renderer_destroy_shader_module(FrRenderer *renderer,
                                       VkShaderModule shader);

// Textures
FrTexture *fr_renderer_create_texture(FrRenderer *renderer,
                                      struct FrAsset *asset);
void fr_renderer_destroy_texture(FrRenderer *renderer, FrTexture *texture);

// Drawing
void fr_draw_sprite(FrRenderer *renderer, FrTexture *texture, FrVec2 pos,
                    FrVec2 size, FrVec4 color);

// Draw a sprite using a scene node's world transform for position and scale.
// Requires fr_node_update_transform to have been called this frame.
struct FrNode;
void fr_draw_sprite_node(FrRenderer *renderer, FrTexture *texture,
                         struct FrNode *node, FrVec2 size, FrVec4 color);

bool fr_renderer_begin_frame(FrRenderer *renderer);
void fr_renderer_end_frame(FrRenderer *renderer);
void fr_renderer_update_projection(FrRenderer *renderer, FrMat4 projection);

VkDevice fr_renderer_get_device(FrRenderer *renderer);
VkQueue fr_renderer_get_graphics_queue(FrRenderer *renderer);

#endif // FR_RENDER_H
