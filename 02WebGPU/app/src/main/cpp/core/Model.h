#pragma once

#include <vector>
#include <iterator>

enum ModelColor {
	MC_WHITE,
	MC_RED,
	MC_GREEN,
	MC_BLUE,
	MC_BLACK,
	MC_POSITION
};

enum ProjectedPlane {
	XY,
	XZ,
	YZ
};

class Mesh;
class Model {
	
public:

	virtual ~Model() = default;
	virtual unsigned int getStride() const = 0;

protected:

	void static GenerateColors(std::vector<float>& vertexBuffer, std::vector<unsigned int>& indexBuffer, unsigned int& stride, ModelColor modelColor);
	void static GenerateUVs(std::vector<float>& vertexBuffer, unsigned int& stride, ProjectedPlane projectedPlane = XY);
	void static PackBuffer(std::vector<float>& vertexBuffer, unsigned int stride);
	void static Rewind(const std::vector<float>& vertexBuffer, std::vector<unsigned int>& indexBuffer, unsigned int stride);

	void static GenerateNormals(std::vector<float>& vertexBuffer, std::vector<unsigned int>& indexBuffer, Model& model, bool& hasNormals, unsigned int& stride, unsigned int startIndex, unsigned int endIndex);
	void static GenerateNormals(std::vector<float>& vertexCords, std::vector<std::array<int, 10>>& faces, std::vector<float>& normalCords);
	void static GenerateTangents(std::vector<float>& vertexBuffer, std::vector<unsigned int>& indexBuffer, Model& model, bool& hasNormals, bool& hasTangents, unsigned int& stride, unsigned int startIndex, unsigned int endIndex);
	void static GenerateTangents(std::vector<float>& vertexCords, std::vector<float>& textureCords, std::vector<float>& normalCords, std::vector<std::array<int, 10>>& faces, std::vector<float>& tangentCords, std::vector<float>& bitangentCords);

	std::vector<Mesh*> m_meshes;

private:

	static std::array<float, 3> Normalize(const std::array<float, 3>& v);
	static std::array<float, 3> Cross(const std::array<float, 3>& p, const std::array<float, 3>& q);
	static float Dot(const std::array<float, 3>& p, const std::array<float, 3>& q);
};