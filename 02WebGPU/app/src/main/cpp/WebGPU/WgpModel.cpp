#include <Shape.h>
#include "WgpModel.h"

WgpModel::WgpModel(WgpModel const& rhs) : m_meshes(rhs.m_meshes) {

}

WgpModel::WgpModel(WgpModel&& rhs) noexcept : m_meshes(std::move(rhs.m_meshes)) {

}

void WgpModel::create(const Shape& shape) {
	m_meshes.emplace_back(shape.getVertexBuffer(), shape.getIndexBuffer());
	markForDelete();
}

void WgpModel::create(const std::vector<float>& vertexBuffer, const std::vector<unsigned int>& indexBuffer) {
	m_meshes.emplace_back(vertexBuffer, indexBuffer);
	markForDelete();
}

void WgpModel::create(const WgpBuffer& vertexBuffer, const WgpBuffer& indexBuffer) {
	m_meshes.emplace_back(vertexBuffer, indexBuffer);
}

void WgpModel::markForDelete(){
	for (WgpMesh& mesh : m_meshes) {
		mesh.markForDelete();
	}
}

void WgpModel::setBindGroupsSlot(const std::string& bindGroupsSlot) {
	for (WgpMesh& mesh : m_meshes) {
		mesh.setBindGroupsSlot(bindGroupsSlot);
	}
}

void WgpModel::setBindGroups(const std::string& bindGroupsName, const std::function <std::vector<WGPUBindGroup>()>& onBindGroups) {
	for (WgpMesh& mesh : m_meshes) {
		mesh.setBindGroups(bindGroupsName, onBindGroups);
	}
}

void WgpModel::addBindGroups(const std::string& bindGroupsName, const std::function <std::vector<WGPUBindGroup>()>& onBindGroups) {
	for (WgpMesh& mesh : m_meshes) {
		mesh.addBindGroups(bindGroupsName, onBindGroups);
	}
}

void WgpModel::addBindGroup(const std::string& bindGroupsName, WGPUBindGroup bindGroup) const {
	for (WgpMesh& mesh : m_meshes) {
		mesh.addBindGroup(bindGroupsName, bindGroup);
	}
}

void WgpModel::addColor(std::array<float, 4> color) {
	for (WgpMesh& mesh : m_meshes) {
		mesh.addColor(color);
	}
}

const std::list<WgpMesh>& WgpModel::getMeshes() const{
	return m_meshes;
}

void WgpModel::draw(const WGPURenderPassEncoder& renderPassEncoder, uint32_t instanceCount) const {
	for (std::list<WgpMesh>::const_iterator it = m_meshes.begin(); it != m_meshes.end(); ++it) {
		(*it).draw(renderPassEncoder, instanceCount);
	}
}

void WgpModel::draw(const WGPURenderBundleEncoder& renderBundleEncoder, uint32_t instanceCount) const {
	for (std::list<WgpMesh>::const_iterator it = m_meshes.begin(); it != m_meshes.end(); ++it) {
		(*it).draw(renderBundleEncoder, instanceCount);
	}
}