#ifndef _MESHCUBE_H
#define _MESHCUBE_H

#include <android/asset_manager.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <vector>
#include <memory>

#include "Camera.h"
#include "Shader.h"
#include "Vector.h"
#include "ModelMatrix.h"
#include "Texture.h"

class MeshCube {

public:

    MeshCube(float width, float height, float depth, const std::string &texture, AAssetManager* assetManager);
    MeshCube(const Vector3f &position, float width, float height, float depth, const std::string &texture, AAssetManager* assetManager);
    MeshCube(const Vector3f &position, float width, float height, float depth, bool generateTexels, const std::string &texture, AAssetManager* assetManager);
    ~MeshCube();

    void setPrecision(int uResolution, int vResolution);
    void buildMesh();
    void buildMesh4Q();
    void draw(const Camera camera);

    void setRotPos(const Vector3f &axis, float degrees, float dx, float dy, float dz);
    void setRotXYZPos(const Vector3f &axisX, float degreesX,
                      const Vector3f &axisY, float degreesY,
                      const Vector3f &axisZ, float degreesZ,
                      float dx, float dy, float dz);

    void rotate(const Vector3f &axis, float degrees);
    void translate(float dx, float dy, float dz);
    void scale(float a, float b, float c);

    std::vector<unsigned int> m_indexBuffer;
    std::vector<Vector3f> m_positions;



    int m_uResolution;
    int m_vResolution;

    float m_width;
    float m_height;
    float m_depth;
    Vector3f m_position;

    bool m_generateNormals;
    bool m_generateTexels;
    bool m_generateTangents;
    bool m_generateNormalDerivatives;

    bool m_isInitialized;
    bool m_hasTexels;
    bool m_hasNormals;
    bool m_hasTangents;
    bool m_hasNormalDerivatives;

    short m_numBuffers;
    unsigned int m_vao;
    unsigned int m_vbo[4];
    unsigned int m_drawCount;

    ModelMatrix m_modelMatrix;

    std::shared_ptr<Shader> m_shader;
    std::shared_ptr<Texture> m_texture;
};
#endif