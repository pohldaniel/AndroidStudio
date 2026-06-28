#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <map>
#include <string>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <numeric>

#include <glm/glm.hpp>

#include "Model.h"
#include "Mesh.h"
#include "Material.h"

struct IndexBufferCreator {

	std::vector<std::array<int, 10>> face;
	std::vector<float> positionCordsIn;
	std::vector<float> normalCordsIn;
	std::vector<float> textureCordsIn;
	std::vector<float> tangentCordsIn;
	std::vector<float> bitangentCordsIn;

	std::vector <float> vertexBufferOut;
	std::vector<unsigned int> indexBufferOut;

	void createIndexBuffer(bool flipWinding = false);

private:
	std::map<int, std::vector<int>> m_vertexCache;
	int addVertex(int hash, const float *pVertex, int stride);
};

class ObjMesh;
class ObjModel : public Model {

	friend ObjMesh;

public:

	ObjModel();
	ObjModel(ObjModel const& rhs);
	ObjModel(ObjModel&& rhs) noexcept;
	ObjModel& operator=(const ObjModel& rhs);
	ObjModel& operator=(ObjModel&& rhs) noexcept;
	~ObjModel() override;

	void loadModel(const char* filename, bool isStacked = false, bool withoutNormals = false, bool generateSmoothNormals = false, bool generateFlatNormals = false, bool generateSmoothTangents = false, bool flipYZ = false, bool flipWinding = false, bool rescale = false);
	void loadModel(const char* filename, const glm::vec3& axis, float degrees, const glm::vec3& translate = glm::vec3(0.0f, 0.0f, 0.0f), float scale = 1.0f, bool isStacked = false, bool withoutNormals = false, bool generateSmoothNormals = false, bool generateFlatNormals = false, bool generateSmoothTangents = false, bool flipYZ = false, bool flipWinding = false, bool rescale = false);
	void loadModelCpu(const char* filename, bool isStacked = false, bool withoutNormals = false, bool generateSmoothNormals = false, bool generateFlatNormals = false, bool generateSmoothTangents = false, bool flipYZ = false, bool flipWinding = false, bool rescale = false);
	void loadModelCpu(const char* filename, const glm::vec3& axis, float degrees, const glm::vec3& translate = glm::vec3(0.0f, 0.0f, 0.0f), float scale = 1.0f, bool asStacked = false, bool withoutNormals = false, bool generateSmoothNormals = false, bool generateFlatNormals = false, bool generateSmoothTangents = false, bool flipYZ = false, bool flipWinding = false, bool rescale = false);

	const glm::vec3& getCenter() const;

	unsigned int getStride() const override;
	const std::string& getMltPath();
	const std::string& getModelDirectory();
	const Mesh* getMesh(unsigned short index = 0u) const;
	const std::vector<Mesh*>& getMeshes() const;
	const std::vector<float>& getVertexBuffer() const;
	const std::vector<unsigned int>& getIndexBuffer() const;
	unsigned int getNumberOfTriangles();
	
	void generateNormals();
	void generateTangents();
	void rewind();

	void generateColors(ModelColor modelColor = MC_WHITE);
	void generateUVs(ProjectedPlane projectedPlane = XY);
	void packBuffer();
	void cleanup();
	
private:

	unsigned int m_numberOfTriangles, m_numberOfMeshes, m_stride;

	bool m_hasTextureCoords, m_hasNormals, m_hasTangents, m_hasMaterial;
	bool m_isStacked;

	std::string m_mltPath;
	std::string m_modelDirectory;
	glm::vec3 m_center;

	std::vector<float> m_vertexBuffer;
	std::vector<unsigned int> m_indexBuffer;

	unsigned int m_drawCount;

	void static GenerateFlatNormals(std::vector<float>& vertexCoords, std::vector<std::array<int, 10>>& face, std::vector<float>& normalCoords);
	void static ReadMaterialFromFile(std::string path, std::string mltLib, std::string mltName, short& index);
	std::string static GetTexturePath(std::string texPath, std::string modelDirectory);
};

class ObjMesh : public Mesh {

	friend ObjModel;

public:

	ObjMesh( ObjModel* model, const std::string& mltName);
	ObjMesh(ObjModel* model);
	ObjMesh(ObjMesh const& rhs);
	ObjMesh(ObjMesh&& rhs) noexcept;
	ObjMesh& operator=(const ObjMesh& rhs);
	ObjMesh& operator=(ObjMesh&& rhs) noexcept;
	~ObjMesh() override;

	short getMaterialIndex() const;
	void setMaterialIndex(short index) const;
	short getTextureIndex() const;
	void setTextureIndex(short index) const;
	const Material& getMaterial() const;
	unsigned int getNumberOfTriangles() const;
	void cleanup();

	const bool hasMaterial() const;
	
private:

	ObjModel* m_model;
	std::string m_mltName;

	mutable short m_textureIndex;
	mutable short m_materialIndex;	
};