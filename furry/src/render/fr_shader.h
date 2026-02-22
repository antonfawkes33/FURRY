#ifndef FR_SHADER_H
#define FR_SHADER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>


typedef struct FrRenderer FrRenderer;

typedef struct FrShader {
  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;
} FrShader;

// Creates a complete shader pipeline, compatible with the renderer's render
// pass. `push_constant_size` allows sending custom float data (e.g. time,
// progress) to the fragment shader.
FrShader *fr_shader_create(FrRenderer *renderer, const uint32_t *vert_code,
                           size_t vert_size, const uint32_t *frag_code,
                           size_t frag_size, uint32_t push_constant_size);

void fr_shader_destroy(FrRenderer *renderer, FrShader *shader);

// Bind a custom shader for subsequent draw calls.
void fr_renderer_bind_shader(FrRenderer *renderer, FrShader *shader);

// Send uniform data to the currently bound shader. Must be called after
// bind_shader.
void fr_renderer_set_shader_push_constants(FrRenderer *renderer,
                                           FrShader *shader, const void *data,
                                           uint32_t size);

#endif // FR_SHADER_H
