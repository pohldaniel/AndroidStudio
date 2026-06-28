#pragma once

#include <iostream>
#include <functional>
#include <numeric>
#include <unordered_map>
#include <map>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/IOStream.hpp>

#include <glm/glm.hpp>

#include "Model.h"
#include "Mesh.h"
#include "Material.h"

//#define ASSIMP_LOAD_FLAGS (aiProcess_JoinIdenticalVertices | aiProcess_RemoveRedundantMaterials | aiProcess_PreTransformVertices | aiProcess_Triangulate)
#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_FindDegenerates | aiProcess_GenUVCoords)

class MemoryIOSystem : public Assimp::IOSystem {

private:

    std::map<std::string, std::vector<char>> virtualFiles;

public:

    void AddFile(const std::string& filename, const std::vector<char>& data) {
        virtualFiles[filename] = data;
    }

    bool Exists(const char* pFile) const override {
        return virtualFiles.find(pFile) != virtualFiles.end();
    }

    char getOsSeparator() const override {
        return '/';
    }

    Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override {
        auto it = virtualFiles.find(pFile);
        if (it == virtualFiles.end()) {
            return nullptr;
        }

        struct MemoryIOStream : public Assimp::IOStream {
            const std::vector<char>& buffer;
            size_t cursor = 0;

            MemoryIOStream(const std::vector<char>& buf) : buffer(buf) {}

            size_t Read(void* pvBuffer, size_t size, size_t count) override {
                size_t bytesToRead = std::min(size * count, buffer.size() - cursor);
                if (bytesToRead > 0) {
                    std::memcpy(pvBuffer, &buffer[cursor], bytesToRead);
                    cursor += bytesToRead;
                }
                return bytesToRead / size;
            }

            size_t Write(const void* pvBuffer, size_t size, size_t count) override { return 0; }
            aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
                if (pOrigin == aiOrigin_SET) cursor = pOffset;
                else if (pOrigin == aiOrigin_CUR) cursor += pOffset;
                else if (pOrigin == aiOrigin_END) cursor = buffer.size() - pOffset;
                return AI_SUCCESS;
            }

            size_t Tell() const override {
                return cursor;
            }

            size_t FileSize() const override {
                return buffer.size();
            }

            void Flush() override {}
        };

        return new MemoryIOStream(it->second);
    }

    void Close(Assimp::IOStream* pStream) override {
        delete pStream;
    }
};

class AssimpMesh;
class AssimpModel : public Model {

	friend AssimpMesh;

public:

	AssimpModel();
	AssimpModel(AssimpModel const& rhs);
	AssimpModel(AssimpModel&& rhs) noexcept;
	AssimpModel& operator=(const AssimpModel& rhs);
	AssimpModel& operator=(AssimpModel&& rhs) noexcept;
	~AssimpModel() override;

	void loadModel(MemoryIOSystem* memoryIOSystem, const char* filename, bool isStacked = false, bool generateNormals = false, bool generateTangents = false, bool flipYZ = false, bool flipWinding = false);
	void loadModel(MemoryIOSystem* memoryIOSystem, const char* filename, const glm::vec3& axis, float degrees, const glm::vec3& translate = glm::vec3(0.0f, 0.0f, 0.0f), float scale = 1.0f, bool isStacked = false, bool generateNormals = false, bool generateTangents = false, bool flipYZ = false, bool flipWinding = false);
	void loadModelCpu(MemoryIOSystem* memoryIOSystem, const char* filename, bool isStacked = false, bool generateNormals = false, bool generateTangents = false, bool flipYZ = false, bool flipWinding = false);
	void loadModelCpu(MemoryIOSystem* memoryIOSystem, const char* filename, const glm::vec3& axis, float degrees, const glm::vec3& translate = glm::vec3(0.0f, 0.0f, 0.0f), float scale = 1.0f, bool isStacked = false, bool generateNormals = false, bool generateTangents = false, bool flipYZ = false, bool flipWinding = false);
	
	const glm::vec3& getCenter() const;

	unsigned int getStride() const override;
	const std::string& getModelDirectory();
	const Mesh* getMesh(unsigned short index = 0u) const;
	const std::vector<Mesh*>& getMeshes() const;
	const std::vector<float>& getVertexBuffer() const;
	const std::vector<unsigned int>& getIndexBuffer() const;
	const unsigned int getNumberOfTriangles() const;

	void generateNormals();
	void rewind();

	void generateColors(ModelColor modelColor = MC_WHITE);
	void generateUVs(ProjectedPlane projectedPlane = XY);
	void packBuffer();
	void cleanup();

private:

	unsigned int m_numberOfTriangles, m_numberOfMeshes, m_stride;

	bool m_hasTextureCords, m_hasNormals, m_hasTangents, m_hasMaterial;
	bool m_isStacked;

	std::string m_modelDirectory;
	glm::vec3 m_center;
	unsigned int m_drawCount;

	std::vector<float> m_vertexBuffer;
	std::vector<unsigned int> m_indexBuffer;

	void static ReadAiMaterial(const aiMaterial* aiMaterial, short& index, const std::string& modelDirectory, const std::string& mltName);
	std::string static GetTexturePath(std::string texPath, std::string modelDirectory);
};

class AssimpMesh : public Mesh {

	friend AssimpModel;

public:

	AssimpMesh(AssimpModel* model);
	AssimpMesh(AssimpMesh const& rhs);
	AssimpMesh(AssimpMesh&& rhs) noexcept;
	AssimpMesh& operator=(const AssimpMesh& rhs);
	AssimpMesh& operator=(AssimpMesh&& rhs) noexcept;
	~AssimpMesh() override;

	short getMaterialIndex() const;
	void setMaterialIndex(short index) const;
	short getTextureIndex() const;
	void setTextureIndex(short index) const;

	const Material& getMaterial() const;
	void cleanup();

	const std::unordered_map<TextureSlot, std::pair<unsigned char*, unsigned int>>& getEmbeddedTextures() const;
	const void removeEmbeddedTexture(TextureSlot textureSlot) const;
	const bool hasMaterial() const;

private:

	AssimpModel* m_model;

	mutable short m_textureIndex;
	mutable short m_materialIndex;
	mutable std::unordered_map<TextureSlot, std::pair<unsigned char*, unsigned int>> m_embeddedTextures;
};