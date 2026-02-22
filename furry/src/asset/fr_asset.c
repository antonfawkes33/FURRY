#include "fr_asset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/fr_log.h"

bool fr_asset_init(void) {
  FR_INFO("Asset system initialized");
  return true;
}

void fr_asset_shutdown(void) { FR_INFO("Asset system shutdown"); }

#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb/stb_image.h"

FrAsset *fr_asset_load(const char *path, FrAssetType type) {
  FrAsset *asset = (FrAsset *)malloc(sizeof(FrAsset));
  if (!asset)
    return NULL;

  memset(asset, 0, sizeof(FrAsset));
  strncpy(asset->path, path, sizeof(asset->path) - 1);
  asset->type = type;
  asset->ref_count = 1;

  if (type == FR_ASSET_TEXTURE) {
    asset->data =
        stbi_load(path, &asset->width, &asset->height, &asset->channels, 4);
    if (!asset->data) {
      FR_ERROR("Failed to load texture: %s", path);
      free(asset);
      return NULL;
    }
    asset->size = (size_t)(asset->width * asset->height * 4);
  } else {
    FILE *file = fopen(path, "rb");
    if (!file) {
      FR_ERROR("Failed to open asset: %s", path);
      free(asset);
      return NULL;
    }

    fseek(file, 0, SEEK_END);
    asset->size = ftell(file);
    fseek(file, 0, SEEK_SET);

    asset->data = malloc(asset->size + 1);
    if (!asset->data) {
      fclose(file);
      free(asset);
      return NULL;
    }

    if (fread(asset->data, 1, asset->size, file) != asset->size) {
      free(asset->data);
      fclose(file);
      free(asset);
      return NULL;
    }
    ((char *)asset->data)[asset->size] = '\0';
    fclose(file);
  }

  FR_INFO("Asset loaded: %s (%zu bytes)", path, asset->size);
  return asset;
}

void fr_asset_unload(FrAsset *asset) {
  if (!asset)
    return;
  if (--asset->ref_count == 0) {
    FR_INFO("Asset unloaded: %s", asset->path);
    if (asset->type == FR_ASSET_TEXTURE) {
      stbi_image_free(asset->data);
    } else {
      free(asset->data);
    }
    free(asset);
  }
}
