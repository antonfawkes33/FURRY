#include "fr_renderer.h"
#include "../asset/fr_asset.h"
#include "../core/fr_log.h"
#include "../platform/fr_platform.h"
#include "../scene/fr_scene.h"
#include "fr_shader.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SWAPCHAIN_IMAGES 3
#define MAX_FRAMES_IN_FLIGHT 2

#define MAX_BATCH_SPRITES 10000
#define MAX_BATCH_VERTICES (MAX_BATCH_SPRITES * 4)
#define MAX_BATCH_INDICES (MAX_BATCH_SPRITES * 6)

struct FrTexture {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
  VkDescriptorSet descriptor_set;
  int width, height;
};

struct FrRenderer {
  VkInstance instance;
  VkSurfaceKHR surface;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkQueue graphics_queue;
  VkQueue present_queue;
  uint32_t graphics_family;
  uint32_t present_family;

  VkSwapchainKHR swapchain;
  VkFormat swapchain_format;
  VkExtent2D swapchain_extent;
  uint32_t image_count;
  VkImage images[MAX_SWAPCHAIN_IMAGES];
  VkImageView image_views[MAX_SWAPCHAIN_IMAGES];

  VkCommandPool command_pool;
  VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];

  VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
  VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];

  VkDescriptorSetLayout descriptor_set_layout;
  VkRenderPass render_pass;
  FrShader *base_shader;
  FrShader *current_shader;
  VkFramebuffer framebuffers[MAX_SWAPCHAIN_IMAGES];

  VkDescriptorPool descriptor_pool;

  // Batch geometry for sprites
  VkBuffer batch_vbo[MAX_FRAMES_IN_FLIGHT];
  VkDeviceMemory batch_vbo_mem[MAX_FRAMES_IN_FLIGHT];
  FrVertex *batch_vbo_mapped[MAX_FRAMES_IN_FLIGHT];

  VkBuffer batch_ibo;
  VkDeviceMemory batch_ibo_mem;

  FrTexture *current_batch_texture;
  uint32_t batch_sprite_count;

  FrMat4 projection;

  uint32_t current_frame;
  uint32_t image_index;
  bool frame_begun;
};

// Internal helpers
static bool create_instance(FrRenderer *renderer, const FrRendererInfo *info);
static bool select_physical_device(FrRenderer *renderer);
static bool create_logical_device(FrRenderer *renderer);
static bool create_swapchain(FrRenderer *renderer, FrWindow *window);
static bool create_image_views(FrRenderer *renderer);
static bool create_render_pass(FrRenderer *renderer);
static bool create_descriptor_set_layout(FrRenderer *renderer);
static bool create_graphics_pipeline(FrRenderer *renderer);
static bool create_framebuffers(FrRenderer *renderer);
static bool create_descriptor_pool(FrRenderer *renderer);
static bool create_command_pool(FrRenderer *renderer);
static bool create_command_buffers(FrRenderer *renderer);
static bool create_command_buffers(FrRenderer *renderer);
static bool create_sync_objects(FrRenderer *renderer);
static bool create_batch_buffers(FrRenderer *renderer);
void fr_renderer_flush(FrRenderer *renderer);

static bool create_image(FrRenderer *renderer, uint32_t width, uint32_t height,
                         VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties, VkImage *image,
                         VkDeviceMemory *image_memory);
static bool create_texture_sampler(FrRenderer *renderer, VkSampler *sampler);
static bool create_texture_image_view(FrRenderer *renderer, VkImage image,
                                      VkFormat format, VkImageView *view);
static void copy_buffer_to_image(FrRenderer *renderer, VkBuffer buffer,
                                 VkImage image, uint32_t width,
                                 uint32_t height);

static uint32_t find_memory_type(FrRenderer *renderer, uint32_t type_filter,
                                 VkMemoryPropertyFlags properties);
static void transition_image_layout(VkCommandBuffer cmd, VkImage image,
                                    VkImageLayout old_layout,
                                    VkImageLayout new_layout);

// Internal helpers

FrRenderer *fr_renderer_init(FrWindow *window, const FrRendererInfo *info) {
  if (!window)
    return NULL;

  struct FrRenderer *renderer =
      (struct FrRenderer *)malloc(sizeof(struct FrRenderer));
  if (!renderer)
    return NULL;
  memset(renderer, 0, sizeof(struct FrRenderer));

  if (!create_instance(renderer, info))
    goto error;

  struct SDL_Window *window_handle = fr_window_get_handle(window);
  if (!window_handle ||
      !SDL_Vulkan_CreateSurface(window_handle, renderer->instance, NULL,
                                &renderer->surface)) {
    FR_ERROR("Failed to create Vulkan surface: %s", SDL_GetError());
    goto error;
  }

  if (!select_physical_device(renderer))
    goto error;
  if (!create_logical_device(renderer))
    goto error;
  if (!create_swapchain(renderer, window))
    goto error;
  if (!create_image_views(renderer))
    goto error;
  if (!create_command_pool(renderer))
    goto error;
  if (!create_command_buffers(renderer))
    goto error;
  if (!create_sync_objects(renderer))
    goto error;
  if (!create_render_pass(renderer))
    goto error;
  if (!create_descriptor_set_layout(renderer))
    goto error;
  if (!create_graphics_pipeline(renderer))
    goto error;
  if (!create_framebuffers(renderer))
    goto error;
  if (!create_descriptor_pool(renderer))
    goto error;
  if (!create_batch_buffers(renderer))
    goto error;

  renderer->projection = fr_mat4_identity();

  FR_INFO("Vulkan renderer initialized");
  return renderer;

error:
  if (renderer)
    fr_renderer_shutdown(renderer);
  return NULL;
}

void fr_renderer_shutdown(FrRenderer *renderer) {
  if (!renderer)
    return;

  if (renderer->device) {
    vkDeviceWaitIdle(renderer->device);
  }

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (renderer->image_available_semaphores[i])
      vkDestroySemaphore(renderer->device,
                         renderer->image_available_semaphores[i], NULL);
    if (renderer->render_finished_semaphores[i])
      vkDestroySemaphore(renderer->device,
                         renderer->render_finished_semaphores[i], NULL);
    if (renderer->in_flight_fences[i])
      vkDestroyFence(renderer->device, renderer->in_flight_fences[i], NULL);
  }

  if (renderer->descriptor_pool)
    vkDestroyDescriptorPool(renderer->device, renderer->descriptor_pool, NULL);

  for (uint32_t i = 0; i < renderer->image_count; i++) {
    if (renderer->framebuffers[i])
      vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
  }

  if (renderer->base_shader)
    fr_shader_destroy(renderer, renderer->base_shader);
  if (renderer->render_pass)
    vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
  if (renderer->descriptor_set_layout)
    vkDestroyDescriptorSetLayout(renderer->device,
                                 renderer->descriptor_set_layout, NULL);

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (renderer->batch_vbo_mapped[i]) {
      vkUnmapMemory(renderer->device, renderer->batch_vbo_mem[i]);
    }
    fr_renderer_destroy_buffer(renderer, renderer->batch_vbo[i],
                               renderer->batch_vbo_mem[i]);
  }
  fr_renderer_destroy_buffer(renderer, renderer->batch_ibo,
                             renderer->batch_ibo_mem);

  if (renderer->command_pool)
    vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);

  for (uint32_t i = 0; i < renderer->image_count; i++) {
    if (renderer->image_views[i])
      vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
  }

  if (renderer->swapchain)
    vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
  if (renderer->device)
    vkDestroyDevice(renderer->device, NULL);
  if (renderer->surface)
    vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
  if (renderer->instance)
    vkDestroyInstance(renderer->instance, NULL);
  free(renderer);
  FR_INFO("Vulkan renderer shutdown");
}

static bool create_instance(struct FrRenderer *renderer,
                            const struct FrRendererInfo *info) {
  VkApplicationInfo app_info = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                .pApplicationName = info->app_name,
                                .applicationVersion = info->app_version,
                                .pEngineName = "FURRY",
                                .engineVersion = VK_MAKE_VERSION(0, 1, 0),
                                .apiVersion = VK_API_VERSION_1_2};

  uint32_t extension_count = 0;
  const char *const *extensions =
      SDL_Vulkan_GetInstanceExtensions(&extension_count);
  if (!extensions) {
    FR_ERROR("Failed to get SDL Vulkan extensions: %s", SDL_GetError());
    return false;
  }

  VkInstanceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = extension_count,
      .ppEnabledExtensionNames = extensions};

  if (vkCreateInstance(&create_info, NULL, &renderer->instance) != VK_SUCCESS) {
    FR_ERROR("Failed to create Vulkan instance");
    return false;
  }
  return true;
}

static bool find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface,
                                uint32_t *graphics, uint32_t *present) {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);
  VkQueueFamilyProperties *queues = (VkQueueFamilyProperties *)malloc(
      sizeof(VkQueueFamilyProperties) * count);
  if (!queues)
    return false;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queues);

  bool found_graphics = false;
  bool found_present = false;

  for (uint32_t i = 0; i < count; i++) {
    if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      *graphics = i;
      found_graphics = true;
    }
    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
    if (present_support) {
      *present = i;
      found_present = true;
    }
    if (found_graphics && found_present)
      break;
  }

  free(queues);
  return found_graphics && found_present;
}

static bool select_physical_device(struct FrRenderer *renderer) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(renderer->instance, &device_count, NULL);
  if (device_count == 0) {
    FR_ERROR("No Vulkan-capable GPUs found");
    return false;
  }

  VkPhysicalDevice *devices =
      (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * device_count);
  if (!devices)
    return false;
  vkEnumeratePhysicalDevices(renderer->instance, &device_count, devices);

  bool device_found = false;
  for (uint32_t i = 0; i < device_count; i++) {
    uint32_t g, p;
    if (find_queue_families(devices[i], renderer->surface, &g, &p)) {
      renderer->physical_device = devices[i];
      renderer->graphics_family = g;
      renderer->present_family = p;
      device_found = true;
      break;
    }
  }

  free(devices);
  if (!device_found) {
    FR_ERROR("No GPU found with required queue families");
    return false;
  }

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(renderer->physical_device, &props);
  FR_INFO("Selected GPU: %s", props.deviceName);

  return true;
}

static bool create_logical_device(struct FrRenderer *renderer) {
  float queue_priority = 1.0f;
  uint32_t families[2] = {renderer->graphics_family, renderer->present_family};
  uint32_t unique_count = (families[0] == families[1]) ? 1 : 2;

  VkDeviceQueueCreateInfo queue_infos[2];
  for (uint32_t i = 0; i < unique_count; i++) {
    queue_infos[i] = (VkDeviceQueueCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = families[i],
        .queueCount = 1,
        .pQueuePriorities = &queue_priority};
  }

  const char *extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkPhysicalDeviceFeatures device_features = {0};

  VkDeviceCreateInfo create_info = {.sType =
                                        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                    .queueCreateInfoCount = unique_count,
                                    .pQueueCreateInfos = queue_infos,
                                    .pEnabledFeatures = &device_features,
                                    .enabledExtensionCount = 1,
                                    .ppEnabledExtensionNames = extensions};

  if (vkCreateDevice(renderer->physical_device, &create_info, NULL,
                     &renderer->device) != VK_SUCCESS) {
    FR_ERROR("Failed to create logical device");
    return false;
  }

  vkGetDeviceQueue(renderer->device, renderer->graphics_family, 0,
                   &renderer->graphics_queue);
  vkGetDeviceQueue(renderer->device, renderer->present_family, 0,
                   &renderer->present_queue);

  return true;
}

static bool create_swapchain(struct FrRenderer *renderer,
                             struct FrWindow *window) {
  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physical_device,
                                            renderer->surface, &caps);

  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device,
                                       renderer->surface, &format_count, NULL);
  VkSurfaceFormatKHR *formats =
      (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      renderer->physical_device, renderer->surface, &format_count, formats);

  VkSurfaceFormatKHR selected_format = formats[0];
  for (uint32_t i = 0; i < format_count; i++) {
    if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      selected_format = formats[i];
      break;
    }
  }
  free(formats);

  uint32_t present_mode_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      renderer->physical_device, renderer->surface, &present_mode_count, NULL);
  VkPresentModeKHR *present_modes =
      (VkPresentModeKHR *)malloc(sizeof(VkPresentModeKHR) * present_mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physical_device,
                                            renderer->surface,
                                            &present_mode_count, present_modes);

  VkPresentModeKHR selected_mode = VK_PRESENT_MODE_FIFO_KHR;
  for (uint32_t i = 0; i < present_mode_count; i++) {
    if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      selected_mode = present_modes[i];
      break;
    }
  }
  free(present_modes);

  int width, height;
  fr_window_get_size(window, &width, &height);
  VkExtent2D extent = {(uint32_t)width, (uint32_t)height};

  renderer->swapchain_format = selected_format.format;
  renderer->swapchain_extent = extent;

  uint32_t image_count = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
    image_count = caps.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = renderer->surface,
      .minImageCount = image_count,
      .imageFormat = selected_format.format,
      .imageColorSpace = selected_format.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .preTransform = caps.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = selected_mode,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE};

  uint32_t families[] = {renderer->graphics_family, renderer->present_family};
  if (families[0] != families[1]) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = families;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  if (vkCreateSwapchainKHR(renderer->device, &create_info, NULL,
                           &renderer->swapchain) != VK_SUCCESS) {
    FR_ERROR("Failed to create swapchain");
    return false;
  }

  vkGetSwapchainImagesKHR(renderer->device, renderer->swapchain,
                          &renderer->image_count, NULL);
  if (renderer->image_count > MAX_SWAPCHAIN_IMAGES)
    renderer->image_count = MAX_SWAPCHAIN_IMAGES;
  vkGetSwapchainImagesKHR(renderer->device, renderer->swapchain,
                          &renderer->image_count, renderer->images);

  return true;
}

static bool create_image_views(struct FrRenderer *renderer) {
  for (uint32_t i = 0; i < renderer->image_count; i++) {
    VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = renderer->images[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = renderer->swapchain_format,
        .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .a = VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};

    if (vkCreateImageView(renderer->device, &create_info, NULL,
                          &renderer->image_views[i]) != VK_SUCCESS) {
      FR_ERROR("Failed to create image view for swapchain image %d", i);
      return false;
    }
  }
  return true;
}

static bool create_command_pool(struct FrRenderer *renderer) {
  VkCommandPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = renderer->graphics_family,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};

  if (vkCreateCommandPool(renderer->device, &pool_info, NULL,
                          &renderer->command_pool) != VK_SUCCESS) {
    FR_ERROR("Failed to create command pool");
    return false;
  }
  return true;
}

static bool create_command_buffers(struct FrRenderer *renderer) {
  VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = renderer->command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT};

  if (vkAllocateCommandBuffers(renderer->device, &alloc_info,
                               renderer->command_buffers) != VK_SUCCESS) {
    FR_ERROR("Failed to allocate command buffers");
    return false;
  }
  return true;
}

static bool create_sync_objects(struct FrRenderer *renderer) {
  VkSemaphoreCreateInfo semaphore_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fence_info = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                  .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(renderer->device, &semaphore_info, NULL,
                          &renderer->image_available_semaphores[i]) !=
            VK_SUCCESS ||
        vkCreateSemaphore(renderer->device, &semaphore_info, NULL,
                          &renderer->render_finished_semaphores[i]) !=
            VK_SUCCESS ||
        vkCreateFence(renderer->device, &fence_info, NULL,
                      &renderer->in_flight_fences[i]) != VK_SUCCESS) {
      FR_ERROR("Failed to create synchronization objects for frame %d", i);
      return false;
    }
  }
  return true;
}

static void transition_image_layout(VkCommandBuffer cmd, VkImage image,
                                    VkImageLayout old_layout,
                                    VkImageLayout new_layout) {
  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};

  VkPipelineStageFlags src_stage;
  VkPipelineStageFlags dst_stage;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;
    src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  } else {
    src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  }

  vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1,
                       &barrier);
}

bool fr_renderer_begin_frame(FrRenderer *renderer) {
  if (renderer->frame_begun)
    return false;

  vkWaitForFences(renderer->device, 1,
                  &renderer->in_flight_fences[renderer->current_frame], VK_TRUE,
                  UINT64_MAX);

  VkResult result = vkAcquireNextImageKHR(
      renderer->device, renderer->swapchain, UINT64_MAX,
      renderer->image_available_semaphores[renderer->current_frame],
      VK_NULL_HANDLE, &renderer->image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    FR_ERROR("Failed to acquire swapchain image");
    return false;
  }

  vkResetFences(renderer->device, 1,
                &renderer->in_flight_fences[renderer->current_frame]);
  vkResetCommandBuffer(renderer->command_buffers[renderer->current_frame], 0);

  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

  if (vkBeginCommandBuffer(renderer->command_buffers[renderer->current_frame],
                           &begin_info) != VK_SUCCESS) {
    FR_ERROR("Failed to begin recording command buffer");
    return false;
  }

  // Render Pass
  VkCommandBuffer cmd = renderer->command_buffers[renderer->current_frame];
  VkClearValue clear_value;
  memset(&clear_value, 0, sizeof(clear_value));
  clear_value.color.float32[0] = 0.94f;
  clear_value.color.float32[1] = 0.45f;
  clear_value.color.float32[2] = 0.65f;
  clear_value.color.float32[3] = 1.0f;

  VkRenderPassBeginInfo render_pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = renderer->render_pass,
      .framebuffer = renderer->framebuffers[renderer->image_index],
      .renderArea = {.offset = {0, 0}, .extent = renderer->swapchain_extent},
      .clearValueCount = 1,
      .pClearValues = &clear_value};

  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {0.0f,
                         0.0f,
                         (float)renderer->swapchain_extent.width,
                         (float)renderer->swapchain_extent.height,
                         0.0f,
                         1.0f};
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor = {{0, 0}, renderer->swapchain_extent};
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderer->current_shader->pipeline);

  renderer->batch_sprite_count = 0;
  renderer->current_batch_texture = NULL;

  renderer->frame_begun = true;
  return true;
}

void fr_renderer_end_frame(FrRenderer *renderer) {
  if (!renderer->frame_begun)
    return;

  fr_renderer_flush(renderer);

  vkCmdEndRenderPass(renderer->command_buffers[renderer->current_frame]);

  if (vkEndCommandBuffer(renderer->command_buffers[renderer->current_frame]) !=
      VK_SUCCESS) {
    FR_ERROR("Failed to record command buffer");
    return;
  }

  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &renderer->image_available_semaphores[renderer->current_frame],
      .pWaitDstStageMask = wait_stages,
      .commandBufferCount = 1,
      .pCommandBuffers = &renderer->command_buffers[renderer->current_frame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &renderer->render_finished_semaphores[renderer->current_frame]};

  if (vkQueueSubmit(renderer->graphics_queue, 1, &submit_info,
                    renderer->in_flight_fences[renderer->current_frame]) !=
      VK_SUCCESS) {
    FR_ERROR("Failed to submit draw command buffer");
    return;
  }

  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &renderer->render_finished_semaphores[renderer->current_frame],
      .swapchainCount = 1,
      .pSwapchains = &renderer->swapchain,
      .pImageIndices = &renderer->image_index};

  vkQueuePresentKHR(renderer->present_queue, &present_info);

  renderer->current_frame =
      (renderer->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
  renderer->frame_begun = false;
}

VkShaderModule fr_renderer_create_shader_module(FrRenderer *renderer,
                                                const uint32_t *code,
                                                size_t size) {
  VkShaderModuleCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = size,
      .pCode = code};

  VkShaderModule shader_module;
  if (vkCreateShaderModule(renderer->device, &create_info, NULL,
                           &shader_module) != VK_SUCCESS) {
    FR_ERROR("Failed to create shader module");
    return VK_NULL_HANDLE;
  }

  return shader_module;
}

void fr_renderer_destroy_shader_module(FrRenderer *renderer,
                                       VkShaderModule shader) {
  if (renderer && renderer->device && shader != VK_NULL_HANDLE) {
    vkDestroyShaderModule(renderer->device, shader, NULL);
  }
}

static uint32_t find_memory_type(struct FrRenderer *renderer,
                                 uint32_t type_filter,
                                 VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(renderer->physical_device,
                                      &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }

  FR_ERROR("Failed to find suitable memory type");
  return 0xFFFFFFFF;
}

bool fr_renderer_create_buffer(FrRenderer *renderer, VkDeviceSize size,
                               VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties,
                               VkBuffer *buffer,
                               VkDeviceMemory *buffer_memory) {
  VkBufferCreateInfo buffer_info = {.sType =
                                        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                    .size = size,
                                    .usage = usage,
                                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

  if (vkCreateBuffer(renderer->device, &buffer_info, NULL, buffer) !=
      VK_SUCCESS) {
    FR_ERROR("Failed to create buffer");
    return false;
  }

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(renderer->device, *buffer, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_reqs.size,
      .memoryTypeIndex =
          find_memory_type(renderer, mem_reqs.memoryTypeBits, properties)};

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL, buffer_memory) !=
      VK_SUCCESS) {
    FR_ERROR("Failed to allocate buffer memory");
    return false;
  }

  vkBindBufferMemory(renderer->device, *buffer, *buffer_memory, 0);
  return true;
}

void fr_renderer_destroy_buffer(FrRenderer *renderer, VkBuffer buffer,
                                VkDeviceMemory memory) {
  if (renderer && renderer->device) {
    if (buffer != VK_NULL_HANDLE)
      vkDestroyBuffer(renderer->device, buffer, NULL);
    if (memory != VK_NULL_HANDLE)
      vkFreeMemory(renderer->device, memory, NULL);
  }
}

void fr_renderer_upload_to_buffer(FrRenderer *renderer, VkDeviceMemory memory,
                                  const void *data, VkDeviceSize size) {
  void *mapped_data;
  vkMapMemory(renderer->device, memory, 0, size, 0, &mapped_data);
  memcpy(mapped_data, data, (size_t)size);
  vkUnmapMemory(renderer->device, memory);
}

VkDevice fr_renderer_get_device(FrRenderer *renderer) {
  return renderer->device;
}
VkQueue fr_renderer_get_graphics_queue(FrRenderer *renderer) {
  return renderer->graphics_queue;
}

static bool create_render_pass(struct FrRenderer *renderer) {
  VkAttachmentDescription color_attachment = {
      .format = renderer->swapchain_format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

  VkAttachmentReference color_attachment_ref = {
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass = {.pipelineBindPoint =
                                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  .colorAttachmentCount = 1,
                                  .pColorAttachments = &color_attachment_ref};

  VkSubpassDependency dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

  VkRenderPassCreateInfo render_pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &color_attachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency};

  if (vkCreateRenderPass(renderer->device, &render_pass_info, NULL,
                         &renderer->render_pass) != VK_SUCCESS) {
    FR_ERROR("Failed to create render pass");
    return false;
  }
  return true;
}

static bool create_descriptor_set_layout(struct FrRenderer *renderer) {
  VkDescriptorSetLayoutBinding sampler_layout_binding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = NULL};

  VkDescriptorSetLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &sampler_layout_binding};

  if (vkCreateDescriptorSetLayout(renderer->device, &layout_info, NULL,
                                  &renderer->descriptor_set_layout) !=
      VK_SUCCESS) {
    FR_ERROR("Failed to create descriptor set layout");
    return false;
  }
  return true;
}

static bool create_graphics_pipeline(struct FrRenderer *renderer) {
  FrAsset *vert_shader_asset =
      fr_asset_load("shaders/spirv/base.vert.spv", FR_ASSET_SCRIPT);
  FrAsset *frag_shader_asset =
      fr_asset_load("shaders/spirv/base.frag.spv", FR_ASSET_SCRIPT);

  if (!vert_shader_asset || !frag_shader_asset) {
    FR_ERROR("Failed to load shaders for pipeline");
    return false;
  }

  renderer->base_shader = fr_shader_create(
      renderer, (uint32_t *)vert_shader_asset->data, vert_shader_asset->size,
      (uint32_t *)frag_shader_asset->data, frag_shader_asset->size,
      sizeof(FrMat4));

  fr_asset_unload(vert_shader_asset);
  fr_asset_unload(frag_shader_asset);

  if (!renderer->base_shader)
    return false;
  renderer->current_shader = renderer->base_shader;

  return true;
}

static bool create_framebuffers(FrRenderer *renderer) {
  for (uint32_t i = 0; i < renderer->image_count; i++) {
    VkImageView attachments[] = {renderer->image_views[i]};

    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderer->render_pass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = renderer->swapchain_extent.width,
        .height = renderer->swapchain_extent.height,
        .layers = 1};

    if (vkCreateFramebuffer(renderer->device, &framebuffer_info, NULL,
                            &renderer->framebuffers[i]) != VK_SUCCESS) {
      FR_ERROR("Failed to create framebuffer %d", i);
      return false;
    }
  }
  return true;
}

static bool create_descriptor_pool(FrRenderer *renderer) {
  VkDescriptorPoolSize pool_size = {
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1000};

  VkDescriptorPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = 1,
      .pPoolSizes = &pool_size,
      .maxSets = 1000};

  if (vkCreateDescriptorPool(renderer->device, &pool_info, NULL,
                             &renderer->descriptor_pool) != VK_SUCCESS) {
    FR_ERROR("Failed to create descriptor pool");
    return false;
  }
  return true;
}

void fr_renderer_update_projection(FrRenderer *renderer, FrMat4 projection) {
  renderer->projection = projection;
}

void fr_renderer_flush(FrRenderer *renderer) {
  if (renderer->batch_sprite_count == 0 || !renderer->current_batch_texture)
    return;

  VkCommandBuffer cmd = renderer->command_buffers[renderer->current_frame];

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          renderer->current_shader->pipeline_layout, 0, 1,
                          &renderer->current_batch_texture->descriptor_set, 0,
                          NULL);

  vkCmdPushConstants(cmd, renderer->current_shader->pipeline_layout,
                     VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(FrMat4),
                     &renderer->projection);

  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(
      cmd, 0, 1, &renderer->batch_vbo[renderer->current_frame], offsets);
  vkCmdBindIndexBuffer(cmd, renderer->batch_ibo, 0, VK_INDEX_TYPE_UINT16);

  vkCmdDrawIndexed(cmd, renderer->batch_sprite_count * 6, 1, 0, 0, 0);

  renderer->batch_sprite_count = 0;
}

static bool create_batch_buffers(FrRenderer *renderer) {
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (!fr_renderer_create_buffer(
            renderer, sizeof(FrVertex) * MAX_BATCH_VERTICES,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &renderer->batch_vbo[i], &renderer->batch_vbo_mem[i])) {
      return false;
    }
    vkMapMemory(renderer->device, renderer->batch_vbo_mem[i], 0,
                sizeof(FrVertex) * MAX_BATCH_VERTICES, 0,
                (void **)&renderer->batch_vbo_mapped[i]);
  }

  if (!fr_renderer_create_buffer(renderer, sizeof(uint16_t) * MAX_BATCH_INDICES,
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 &renderer->batch_ibo,
                                 &renderer->batch_ibo_mem)) {
    return false;
  }

  uint16_t *indices;
  vkMapMemory(renderer->device, renderer->batch_ibo_mem, 0,
              sizeof(uint16_t) * MAX_BATCH_INDICES, 0, (void **)&indices);
  for (uint32_t i = 0; i < MAX_BATCH_SPRITES; i++) {
    indices[i * 6 + 0] = i * 4 + 0;
    indices[i * 6 + 1] = i * 4 + 1;
    indices[i * 6 + 2] = i * 4 + 2;
    indices[i * 6 + 3] = i * 4 + 2;
    indices[i * 6 + 4] = i * 4 + 3;
    indices[i * 6 + 5] = i * 4 + 0;
  }
  vkUnmapMemory(renderer->device, renderer->batch_ibo_mem);

  return true;
}

FrTexture *fr_renderer_create_texture(FrRenderer *renderer, FrAsset *asset) {
  if (!asset || asset->type != FR_ASSET_TEXTURE)
    return NULL;

  FrTexture *texture = (FrTexture *)malloc(sizeof(FrTexture));
  if (!texture)
    return NULL;

  texture->width = asset->width;
  texture->height = asset->height;

  VkDeviceSize image_size = (VkDeviceSize)asset->width * asset->height * 4;
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;

  if (!fr_renderer_create_buffer(renderer, image_size,
                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 &staging_buffer, &staging_buffer_memory)) {
    free(texture);
    return NULL;
  }

  fr_renderer_upload_to_buffer(renderer, staging_buffer_memory, asset->data,
                               (size_t)image_size);

  if (!create_image(renderer, (uint32_t)asset->width, (uint32_t)asset->height,
                    VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texture->image,
                    &texture->memory)) {
    fr_renderer_destroy_buffer(renderer, staging_buffer, staging_buffer_memory);
    free(texture);
    return NULL;
  }

  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  VkCommandBuffer cmd = renderer->command_buffers[0];
  vkBeginCommandBuffer(cmd, &begin_info);

  transition_image_layout(cmd, texture->image, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                              .commandBufferCount = 1,
                              .pCommandBuffers = &cmd};
  vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(renderer->graphics_queue);

  copy_buffer_to_image(renderer, staging_buffer, texture->image,
                       (uint32_t)asset->width, (uint32_t)asset->height);

  vkBeginCommandBuffer(cmd, &begin_info);
  transition_image_layout(cmd, texture->image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vkEndCommandBuffer(cmd);
  vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(renderer->graphics_queue);

  fr_renderer_destroy_buffer(renderer, staging_buffer, staging_buffer_memory);

  if (!create_texture_image_view(renderer, texture->image,
                                 VK_FORMAT_R8G8B8A8_SRGB, &texture->view)) {
    fr_renderer_destroy_texture(renderer, texture);
    return NULL;
  }

  if (!create_texture_sampler(renderer, &texture->sampler)) {
    fr_renderer_destroy_texture(renderer, texture);
    return NULL;
  }

  VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = renderer->descriptor_pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &renderer->descriptor_set_layout};

  if (vkAllocateDescriptorSets(renderer->device, &alloc_info,
                               &texture->descriptor_set) != VK_SUCCESS) {
    fr_renderer_destroy_texture(renderer, texture);
    return NULL;
  }

  VkDescriptorImageInfo image_info = {
      .sampler = texture->sampler,
      .imageView = texture->view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

  VkWriteDescriptorSet descriptor_write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = texture->descriptor_set,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .pImageInfo = &image_info};

  vkUpdateDescriptorSets(renderer->device, 1, &descriptor_write, 0, NULL);

  return texture;
}

void fr_renderer_destroy_texture(FrRenderer *renderer, FrTexture *texture) {
  if (!renderer || !texture)
    return;

  if (texture->sampler)
    vkDestroySampler(renderer->device, texture->sampler, NULL);
  if (texture->view)
    vkDestroyImageView(renderer->device, texture->view, NULL);
  if (texture->image)
    vkDestroyImage(renderer->device, texture->image, NULL);
  if (texture->memory)
    vkFreeMemory(renderer->device, texture->memory, NULL);

  free(texture);
}

void fr_draw_sprite(FrRenderer *renderer, FrTexture *texture, FrVec2 pos,
                    FrVec2 size, FrVec4 color) {
  if (!renderer->frame_begun || !texture)
    return;

  if (renderer->current_batch_texture != texture ||
      renderer->batch_sprite_count >= MAX_BATCH_SPRITES) {
    fr_renderer_flush(renderer);
    renderer->current_batch_texture = texture;
  }

  FrVertex *ptr = &renderer->batch_vbo_mapped[renderer->current_frame]
                                             [renderer->batch_sprite_count * 4];

  ptr[0].pos = (FrVec2){pos.x, pos.y};
  ptr[0].color = color;
  ptr[0].uv = (FrVec2){0.0f, 0.0f};

  ptr[1].pos = (FrVec2){pos.x + size.x, pos.y};
  ptr[1].color = color;
  ptr[1].uv = (FrVec2){1.0f, 0.0f};

  ptr[2].pos = (FrVec2){pos.x + size.x, pos.y + size.y};
  ptr[2].color = color;
  ptr[2].uv = (FrVec2){1.0f, 1.0f};

  ptr[3].pos = (FrVec2){pos.x, pos.y + size.y};
  ptr[3].color = color;
  ptr[3].uv = (FrVec2){0.0f, 1.0f};

  renderer->batch_sprite_count++;
}

void fr_draw_sprite_node(FrRenderer *renderer, FrTexture *texture,
                         struct FrNode *node, FrVec2 size, FrVec4 color) {
  if (!node)
    return;

  // Extract world position from column 3 of the world_transform matrix
  FrVec2 world_pos = fr_node_get_world_position(node);

  // Extract world scale (length of basis vectors, handles rotation correctly)
  FrVec2 world_scale = fr_node_get_world_scale(node);

  FrVec2 final_size = {size.x * world_scale.x, size.y * world_scale.y};

  fr_draw_sprite(renderer, texture, world_pos, final_size, color);
}

static bool create_image(FrRenderer *renderer, uint32_t width, uint32_t height,
                         VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties, VkImage *image,
                         VkDeviceMemory *image_memory) {
  VkImageCreateInfo image_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                  .imageType = VK_IMAGE_TYPE_2D,
                                  .extent.width = width,
                                  .extent.height = height,
                                  .extent.depth = 1,
                                  .mipLevels = 1,
                                  .arrayLayers = 1,
                                  .format = format,
                                  .tiling = tiling,
                                  .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                  .usage = usage,
                                  .samples = VK_SAMPLE_COUNT_1_BIT,
                                  .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

  if (vkCreateImage(renderer->device, &image_info, NULL, image) != VK_SUCCESS) {
    FR_ERROR("Failed to create image");
    return false;
  }

  VkMemoryRequirements mem_reqs;
  vkGetImageMemoryRequirements(renderer->device, *image, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_reqs.size,
      .memoryTypeIndex =
          find_memory_type(renderer, mem_reqs.memoryTypeBits, properties)};

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL, image_memory) !=
      VK_SUCCESS) {
    FR_ERROR("Failed to allocate image memory");
    return false;
  }

  vkBindImageMemory(renderer->device, *image, *image_memory, 0);
  return true;
}

static bool create_texture_image_view(FrRenderer *renderer, VkImage image,
                                      VkFormat format, VkImageView *view) {
  VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1};

  if (vkCreateImageView(renderer->device, &view_info, NULL, view) !=
      VK_SUCCESS) {
    FR_ERROR("Failed to create image view");
    return false;
  }
  return true;
}

static bool create_texture_sampler(FrRenderer *renderer, VkSampler *sampler) {
  VkSamplerCreateInfo sampler_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 1.0f,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR};

  if (vkCreateSampler(renderer->device, &sampler_info, NULL, sampler) !=
      VK_SUCCESS) {
    FR_ERROR("Failed to create texture sampler");
    return false;
  }
  return true;
}

static void copy_buffer_to_image(FrRenderer *renderer, VkBuffer buffer,
                                 VkImage image, uint32_t width,
                                 uint32_t height) {
  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  VkCommandBuffer cmd = renderer->command_buffers[0];
  vkBeginCommandBuffer(cmd, &begin_info);

  VkBufferImageCopy region = {.bufferOffset = 0,
                              .bufferRowLength = 0,
                              .bufferImageHeight = 0,
                              .imageSubresource.aspectMask =
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                              .imageSubresource.mipLevel = 0,
                              .imageSubresource.baseArrayLayer = 0,
                              .imageSubresource.layerCount = 1,
                              .imageOffset = {0, 0, 0},
                              .imageExtent = {width, height, 1}};

  vkCmdCopyBufferToImage(cmd, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                              .commandBufferCount = 1,
                              .pCommandBuffers = &cmd};
  vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(renderer->graphics_queue);
}

// --- Shader API Implementation ---

FrShader *fr_shader_create(FrRenderer *renderer, const uint32_t *vert_code,
                           size_t vert_size, const uint32_t *frag_code,
                           size_t frag_size, uint32_t push_constant_size) {
  if (!renderer)
    return NULL;

  FrShader *shader = (FrShader *)malloc(sizeof(FrShader));
  if (!shader)
    return NULL;

  VkShaderModule vert_module =
      fr_renderer_create_shader_module(renderer, vert_code, vert_size);
  VkShaderModule frag_module =
      fr_renderer_create_shader_module(renderer, frag_code, frag_size);

  if (!vert_module || !frag_module) {
    if (vert_module)
      vkDestroyShaderModule(renderer->device, vert_module, NULL);
    if (frag_module)
      vkDestroyShaderModule(renderer->device, frag_module, NULL);
    free(shader);
    return NULL;
  }

  VkPipelineShaderStageCreateInfo shader_stages[] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = vert_module,
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = frag_module,
       .pName = "main"}};

  VkVertexInputBindingDescription binding_description = {
      .binding = 0,
      .stride = sizeof(FrVertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

  VkVertexInputAttributeDescription attribute_descriptions[] = {
      {.binding = 0,
       .location = 0,
       .format = VK_FORMAT_R32G32_SFLOAT,
       .offset = offsetof(FrVertex, pos)},
      {.binding = 0,
       .location = 1,
       .format = VK_FORMAT_R32G32B32A32_SFLOAT,
       .offset = offsetof(FrVertex, color)},
      {.binding = 0,
       .location = 2,
       .format = VK_FORMAT_R32G32_SFLOAT,
       .offset = offsetof(FrVertex, uv)}};

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding_description,
      .vertexAttributeDescriptionCount = 3,
      .pVertexAttributeDescriptions = attribute_descriptions};

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE};

  VkPipelineViewportStateCreateInfo viewport_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1};

  VkPipelineRasterizationStateCreateInfo rasterizer = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .lineWidth = 1.0f,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE};

  VkPipelineMultisampleStateCreateInfo multisampling = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable = VK_FALSE,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

  VkPipelineColorBlendAttachmentState color_blend_attachment = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD};

  VkPipelineColorBlendStateCreateInfo color_blending = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment};

  VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                     VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states};

  VkPushConstantRange push_constant_range = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .offset = 0,
      .size = push_constant_size};

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &renderer->descriptor_set_layout,
      .pushConstantRangeCount = push_constant_size > 0 ? 1 : 0,
      .pPushConstantRanges =
          push_constant_size > 0 ? &push_constant_range : NULL};

  if (vkCreatePipelineLayout(renderer->device, &pipeline_layout_info, NULL,
                             &shader->pipeline_layout) != VK_SUCCESS) {
    FR_ERROR("Failed to create unified pipeline layout");
    vkDestroyShaderModule(renderer->device, frag_module, NULL);
    vkDestroyShaderModule(renderer->device, vert_module, NULL);
    free(shader);
    return NULL;
  }

  VkGraphicsPipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pColorBlendState = &color_blending,
      .pDynamicState = &dynamic_state_info,
      .layout = shader->pipeline_layout,
      .renderPass = renderer->render_pass,
      .subpass = 0};

  if (vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1,
                                &pipeline_info, NULL,
                                &shader->pipeline) != VK_SUCCESS) {
    FR_ERROR("Failed to create graphics pipeline for shader");
    vkDestroyPipelineLayout(renderer->device, shader->pipeline_layout, NULL);
    vkDestroyShaderModule(renderer->device, frag_module, NULL);
    vkDestroyShaderModule(renderer->device, vert_module, NULL);
    free(shader);
    return NULL;
  }

  vkDestroyShaderModule(renderer->device, frag_module, NULL);
  vkDestroyShaderModule(renderer->device, vert_module, NULL);

  return shader;
}

void fr_shader_destroy(FrRenderer *renderer, FrShader *shader) {
  if (!renderer || !shader)
    return;
  if (shader->pipeline)
    vkDestroyPipeline(renderer->device, shader->pipeline, NULL);
  if (shader->pipeline_layout)
    vkDestroyPipelineLayout(renderer->device, shader->pipeline_layout, NULL);
  free(shader);
}

void fr_renderer_bind_shader(FrRenderer *renderer, FrShader *shader) {
  if (!renderer || !renderer->frame_begun || !shader)
    return;
  fr_renderer_flush(renderer); // Flush before changing pipeline
  renderer->current_shader = shader;
  vkCmdBindPipeline(renderer->command_buffers[renderer->current_frame],
                    VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline);
}

void fr_renderer_set_shader_push_constants(FrRenderer *renderer,
                                           FrShader *shader, const void *data,
                                           uint32_t size) {
  if (!renderer || !renderer->frame_begun || !shader || !data)
    return;
  vkCmdPushConstants(renderer->command_buffers[renderer->current_frame],
                     shader->pipeline_layout,
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                     0, size, data);
}
