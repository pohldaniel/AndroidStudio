#ifndef TEXTURE_H
#define TEXTURE_H

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <android/asset_manager.h>
#include <string>



class Texture{

public:
    Texture(const std::string& fileName, AAssetManager* assetManager);

    void setTexture(const std::string& fileName, AAssetManager* assetManager);
    virtual ~Texture();

    void bind(unsigned int unit);
    void unbind();

    GLuint m_texture;
};

#endif