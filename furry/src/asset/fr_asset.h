#ifndef FR_ASSET_H
#define FR_ASSET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  FR_ASSET_TEXTURE,
  FR_ASSET_SCRIPT,
  FR_ASSET_AUDIO,
  FR_ASSET_FONT
} FrAssetType;

typedef struct FrAsset {
  char path[256];
  FrAssetType type;
  void *data;
  size_t size;
  uint32_t ref_count;

  // Texture specific
  int width, height, channels;
} FrAsset;

bool fr_asset_init(void);
void fr_asset_shutdown(void);

FrAsset *fr_asset_load(const char *path, FrAssetType type);
void fr_asset_unload(FrAsset *asset);

#endif // FR_ASSET_H
