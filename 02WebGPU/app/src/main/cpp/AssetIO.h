#include <fstream>
#include <filesystem>
#include <android/asset_manager.h>

struct AssetIO{
    static void Init(AAssetManager* assetManager);
    static void LoadAsset(const char* fileName, uint8_t*& content, uint32_t& size);
    static void Free(uint8_t*& content);

private:

    static AAssetManager* AssetManager;

};