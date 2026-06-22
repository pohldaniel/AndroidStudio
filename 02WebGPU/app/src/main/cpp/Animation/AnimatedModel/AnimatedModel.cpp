#include <iostream>
#include <fstream>
#include <memory>

#include "AnimatedModel.h"

AnimatedModel::AnimatedModel()  {
	m_animator = std::make_shared<Animator>(this);
	m_modelMatrix = ModelMatrix();
}

void AnimatedModel::loadModel(const std::string &filename, const std::string &texture, AAssetManager* assetManager){
	
	ColladaLoader loader(filename, assetManager);
	m_meshes.push_back(std::make_shared<AnimatedMesh>(loader));
	m_shader = std::make_shared<AnimationShader>("animationES30.vert", "animationES30.frag", assetManager);
	m_animator->addAnimation(loader);
	m_texture = std::make_shared<Texture>(texture, assetManager);
}

void AnimatedModel::update(double elapsedTime){
	m_animator->update(elapsedTime);
}

void AnimatedModel::draw(Camera camera){
	m_shader->bind();
	for (auto mesh : m_meshes){
		m_shader->update(*this, camera, mesh->getBoneArray());
		m_texture->bind(0);
		mesh->draw();
		m_texture->unbind();
	}
	m_shader->unbind();
}

void AnimatedModel::setRotPos(const Vector3f &axis, float degrees, float dx, float dy, float dz) {
	m_modelMatrix.setRotPos(axis, degrees, dx, dy, dz);
}

void AnimatedModel::setRotXYZPos(const Vector3f &axisX, float degreesX,
	const Vector3f &axisY, float degreesY,
	const Vector3f &axisZ, float degreesZ,
	float dx, float dy, float dz) {
	m_modelMatrix.setRotXYZPos(axisX, degreesX, axisY, degreesY, axisZ, degreesZ, dx, dy, dz);
}

void AnimatedModel::rotate(const Vector3f &axis, float degrees) {
	m_modelMatrix.rotate(axis, degrees);
}

void AnimatedModel::translate(float dx, float dy, float dz) {
	m_modelMatrix.translate(dx, dy, dz);
}

void AnimatedModel::scale(float a, float b, float c) {
	m_modelMatrix.scale(a, b, c);
}

const Matrix4f &AnimatedModel::getTransformationMatrix() const {
	return m_modelMatrix.getTransformationMatrix();
}

const Matrix4f &AnimatedModel::getInvTransformationMatrix() const {
	return m_modelMatrix.getInvTransformationMatrix();
}
