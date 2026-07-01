#include "AssetIO.h"
#include "Logging.h"

AAssetManager* AssetIO::AssetManager = nullptr;
void AssetIO::Init(AAssetManager* assetManager){
    AssetManager = assetManager;
}

void AssetIO::LoadAsset(const char* fileName, uint8_t*& content, uint32_t& size){
    AAsset* asset = AAssetManager_open(AssetManager, fileName, AASSET_MODE_BUFFER);
    size = AAsset_getLength(asset);

    content = new uint8_t[size];
    AAsset_read(asset, content, size);
    AAsset_close(asset);
}

void AssetIO::Free(uint8_t*& content){
    delete[] content;
}