#ifndef ANIMATION_SHADER_H
#define ANIMATION_SHADER_H

#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>


#include <string>
#include <vector>

#include "../../core/Camera.h"
#include "../../core/Shader.h"

static const unsigned int MAX_JOINTS = 50;
class AnimatedModel;

class AnimationShader : public Shader{

public:
	AnimationShader(const std::string& vertex, const std::string& fragment, AAssetManager* assetManager);
	virtual ~AnimationShader();

	void update(const AnimatedModel& model, const Camera& camera, std::vector<Matrix4f> jointVector);
	void bind();
	void unbind();
};
#endif