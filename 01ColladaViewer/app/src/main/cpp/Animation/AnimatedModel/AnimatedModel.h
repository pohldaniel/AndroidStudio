#ifndef ANIMATEDMODEL_H
#define ANIMATEDMODEL_H

#include <vector>

#include "../../core/Camera.h"
#include "../../core/ModelMatrix.h"

#include "AnimatedMesh.h"
#include "../../core/Texture.h"
#include "../Animator/Animator.h"
#include "../Render/AnimationShader.h"



class AnimatedModel : public std::enable_shared_from_this<AnimatedModel>{

public:

	AnimatedModel();
	virtual ~AnimatedModel() {}

	const Matrix4f &getTransformationMatrix() const;
	const Matrix4f &getInvTransformationMatrix() const;

	void setRotPos(const Vector3f &axis, float degrees, float dx, float dy, float dz);
	void setRotXYZPos(const Vector3f &axisX, float degreesX,
		const Vector3f &axisY, float degreesY,
		const Vector3f &axisZ, float degreesZ,
		float dx, float dy, float dz);

	void rotate(const Vector3f &axis, float degrees);
	void translate(float dx, float dy, float dz);
	void scale(float a, float b, float c);

	void loadModel(const std::string &filename, const std::string &texture, AAssetManager* assetManager);
	
	void update(double elapsedTime);
	void draw(Camera camera);

	std::shared_ptr<Animator> getAnimator() { return m_animator; }

	std::shared_ptr<AnimationShader> m_shader;
	std::vector<std::shared_ptr<AnimatedMesh>> m_meshes;
	std::shared_ptr<Texture> m_texture;
	std::shared_ptr<Animator> m_animator;

private:
	ModelMatrix m_modelMatrix;



};
#endif