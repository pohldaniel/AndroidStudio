#include <iostream>
#include <fstream>
#include <android/log.h>

#include "AnimationShader.h"
#include "../AnimatedModel/AnimatedModel.h"

AnimationShader::AnimationShader(const std::string& vertex, const std::string& fragment, AAssetManager* assetManager) : Shader(vertex, fragment, assetManager) {

}

void AnimationShader::update(const AnimatedModel& model, const Camera& camera, std::vector<Matrix4f> jointVector){

	loadMatrixArray("jointTransforms", jointVector, jointVector.size() < MAX_JOINTS ? jointVector.size() : MAX_JOINTS);
	loadMatrix("u_model", model.getTransformationMatrix());
	loadMatrix("u_view", camera.getViewMatrix());
	loadMatrix("u_projection", camera.getProjectionMatrix());
}

void AnimationShader::bind(){
	glUseProgram(m_program);
}

void AnimationShader::unbind(){
	glUseProgram(0);
}

AnimationShader::~AnimationShader(){
	glDeleteProgram(m_program);
}