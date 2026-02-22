#include <furry.h>
#include <math.h>
#include <stdio.h>

// Phase 2 demo: Scene Graph & Transform Hierarchy
// Two sprites: a parent "star" at screen center, and a child "moon" that orbits
// it. The parent itself pulses in scale. This proves the full TRS hierarchy.

#define SCREEN_W 1280
#define SCREEN_H 720
#define PI 3.14159265358979323846f

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (!fr_platform_init()) {
    printf("Failed to init platform\n");
    return 1;
  }

  FrPlatformInfo win_info = {.title = "FURRY — Phase 2: Scene Graph",
                             .width = SCREEN_W,
                             .height = SCREEN_H,
                             .fullscreen = false};
  FrWindow *window = fr_window_create(&win_info);
  if (!window) {
    fr_platform_shutdown();
    return 1;
  }

  FrRendererInfo renderer_info = {.app_name = "FURRY Phase 2",
                                  .app_version = VK_MAKE_VERSION(0, 2, 0)};
  FrRenderer *renderer = fr_renderer_init(window, &renderer_info);
  if (!renderer) {
    fr_window_destroy(window);
    fr_platform_shutdown();
    return 1;
  }

  // --- Ortho projection (pixel coordinates, Y-down) ---
  FrMat4 proj =
      fr_mat4_ortho(0.0f, (float)SCREEN_W, (float)SCREEN_H, 0.0f, -1.0f, 1.0f);
  fr_renderer_update_projection(renderer, proj);

  // --- Assets ---
  if (!fr_asset_init()) {
    FR_ERROR("Asset system failed to init");
    goto cleanup;
  }
  FrAsset *asset = fr_asset_load("assets/test_character.png", FR_ASSET_TEXTURE);
  if (!asset) {
    FR_ERROR("Failed to load test_character.png");
    goto cleanup;
  }
  FrTexture *tex = fr_renderer_create_texture(renderer, asset);
  fr_asset_unload(asset);
  if (!tex) {
    FR_ERROR("Failed to create texture");
    goto cleanup;
  }

  // -- Sprite size for both sprites (pixels) --
  FrVec2 sprite_size = {128.0f, 128.0f};
  FrVec2 half_sprite = {sprite_size.x * 0.5f, sprite_size.y * 0.5f};

  // --- Scene graph ---
  // Root "planet" node — stays at screen center, rotates over time
  FrNode *planet = fr_node_create("planet");
  fr_node_set_position(planet, fr_vec2((float)SCREEN_W * 0.5f - half_sprite.x,
                                       (float)SCREEN_H * 0.5f - half_sprite.y));

  // Child "moon" node — offset 200 px to the right in parent space
  FrNode *moon = fr_node_create("moon");
  fr_node_set_position(moon, fr_vec2(200.0f, 0.0f));
  fr_node_set_scale(moon, fr_vec2(0.5f, 0.5f)); // moons are smaller
  fr_node_set_parent(moon, planet);

  // --- Time ---
  FrTime time;
  fr_time_init(&time);
  float elapsed = 0.0f;

  while (!fr_window_should_close(window)) {
    fr_window_poll_events(window);
    fr_time_update(&time);
    elapsed += time.delta;

    if (fr_key_pressed(FR_KEY_ESCAPE))
      break;

    // Animate: planet rotates, pulses slightly in scale
    fr_node_set_rotation(planet, elapsed * 1.0f); // 1 rad/s orbit
    float pulse = 1.0f + 0.1f * sinf(elapsed * 3.0f);
    fr_node_set_scale(planet, fr_vec2(pulse, pulse));

    // Resolve world transforms from root downward (one call covers moon too)
    fr_node_update_transform(planet);

    if (!fr_window_is_minimized(window)) {
      if (fr_renderer_begin_frame(renderer)) {
        // Draw parent (planet)
        fr_draw_sprite_node(renderer, tex, planet, sprite_size,
                            fr_vec4(1.0f, 0.9f, 0.4f, 1.0f)); // golden

        // Draw child (moon) — position is auto-derived from hierarchy
        fr_draw_sprite_node(renderer, tex, moon, sprite_size,
                            fr_vec4(0.7f, 0.8f, 1.0f, 1.0f)); // icy blue

        fr_renderer_end_frame(renderer);
      }
    }
  }

  fr_node_destroy(moon);
  fr_node_destroy(planet);
  fr_renderer_destroy_texture(renderer, tex);

cleanup:
  fr_asset_shutdown();
  fr_renderer_shutdown(renderer);
  fr_window_destroy(window);
  fr_platform_shutdown();
  return 0;
}
