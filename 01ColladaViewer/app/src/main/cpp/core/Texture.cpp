#include "Texture.h"
#include "../stb/stb_image.h"

Texture::Texture(const std::string & fileName, AAssetManager* assetManager){
    setTexture(fileName, assetManager);
}

void Texture::setTexture(const std::string & fileName, AAssetManager* assetManager){

    uint32_t width, height, numCompontents;

    AAsset* asset = AAssetManager_open(assetManager, fileName.c_str(), AASSET_MODE_BUFFER);
    long length = AAsset_getLength(asset);

    stbi_uc* fileContent = new unsigned char[length];
    AAsset_read(asset, fileContent, length);
    AAsset_close(asset);
    unsigned char* imageData = stbi_load_from_memory(fileContent, length,
            reinterpret_cast<int*>(&width),
            reinterpret_cast<int*>(&height), reinterpret_cast<int*>(&numCompontents), 4);

    if (imageData == NULL) {
        return;
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(imageData);
}

Texture::~Texture(){
    glDeleteTextures(1, &m_texture);
}

void Texture::bind(unsigned int unit){
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
