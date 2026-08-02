#include <iostream>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../AssimpModel.h"
#include "../BinaryIO.h"
#include "Animation.h"

#include "AssetIO.h"

void AnimationTrack::findKeyFrameIndex(float time, size_t& index) const {
	if (time < 0.0f)
		time = 0.0f;

	if (index >= m_keyFrames.size())
		index = m_keyFrames.size() - 1;

	while (index && time < m_keyFrames[index].m_time)
		--index;

	while (index < m_keyFrames.size() - 1 && time >= m_keyFrames[index + 1].m_time)
		++index;
} 

Animation::Animation() : m_length(0.0f) {

}

Animation::~Animation() {

}

void Animation::loadAnimation(const std::string& filename) {
	Utils::MdlcIO mdlcIO;
	mdlcIO.anicToBuffer(filename.c_str(), m_animationName, m_length, m_tracks);
}

void Animation::loadAnimationAssimp(const std::string& filename, const std::string& sourceName, const std::string& destName, unsigned int startTick, unsigned int endTick) {
	uint8_t* data; uint32_t size;
	AssetIO::LoadAsset(filename.c_str(), data, size);

	MemoryIOSystem* memoryIOSystem = new MemoryIOSystem();
	memoryIOSystem->AddFile(filename, std::vector<char>{data, data + size});

	Assimp::Importer importer;
	importer.SetIOHandler(memoryIOSystem);
	const aiScene* aiScene = importer.ReadFile(filename, 0u);

	if (!aiScene) {
		std::cout << filename << "  " << importer.GetErrorString() << std::endl;
		AssetIO::Free(data);
		return;
	}

	for (unsigned int i = 0; i < aiScene->mNumAnimations; i++) {
		const aiAnimation* aiAnimation = aiScene->mAnimations[i];

		if (sourceName != aiAnimation->mName.data) {
			continue;
		}

		m_animationName = destName;
		m_length = (startTick != 0u || endTick != 0u) ? (endTick- startTick) / aiAnimation->mTicksPerSecond : aiAnimation->mDuration / aiAnimation->mTicksPerSecond;
		m_tracks.clear();

		for (unsigned int c = 0; c < aiAnimation->mNumChannels; c++) {
			AnimationTrack* newTrack = createTrack(aiAnimation->mChannels[c]->mNodeName.data);

			newTrack->m_channelMask = CHANNEL_POSITION + CHANNEL_ROTATION + CHANNEL_SCALE;
			size_t numKeyFrames = std::max(aiAnimation->mChannels[c]->mNumPositionKeys, std::max(aiAnimation->mChannels[c]->mNumRotationKeys, aiAnimation->mChannels[c]->mNumScalingKeys));

			glm::vec3 prevPosition = glm::vec3(0.0f, 0.0f, 0.0f);
			glm::vec3 prevScale = glm::vec3(1.0f, 1.0f, 1.0f);
			glm::quat prevRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			float timeOffset = 0.0f;

			for (size_t j = 0; j < numKeyFrames; ++j) {
				float time = numKeyFrames == aiAnimation->mChannels[c]->mNumPositionKeys ? aiAnimation->mChannels[c]->mPositionKeys[j].mTime :
				             numKeyFrames == aiAnimation->mChannels[c]->mNumRotationKeys ? aiAnimation->mChannels[c]->mRotationKeys[j].mTime :
				             aiAnimation->mChannels[c]->mScalingKeys[j].mTime;

				if (j == 0)
					timeOffset = time;

				time -= timeOffset;

				if (j < aiAnimation->mChannels[c]->mNumPositionKeys)
					prevPosition = glm::vec3(aiAnimation->mChannels[c]->mPositionKeys[j].mValue.x, aiAnimation->mChannels[c]->mPositionKeys[j].mValue.y, aiAnimation->mChannels[c]->mPositionKeys[j].mValue.z);

				if (j < aiAnimation->mChannels[c]->mNumScalingKeys)
					prevScale = glm::vec3(aiAnimation->mChannels[c]->mScalingKeys[j].mValue.x, aiAnimation->mChannels[c]->mScalingKeys[j].mValue.y, aiAnimation->mChannels[c]->mScalingKeys[j].mValue.z);

				if (j < aiAnimation->mChannels[c]->mNumRotationKeys)
					prevRot =  glm::quat(aiAnimation->mChannels[c]->mRotationKeys[j].mValue.w, aiAnimation->mChannels[c]->mRotationKeys[j].mValue.x, aiAnimation->mChannels[c]->mRotationKeys[j].mValue.y, aiAnimation->mChannels[c]->mRotationKeys[j].mValue.z);

				if ((startTick != 0u || endTick != 0u) && (time <= startTick || endTick <= time))
					continue;

				newTrack->m_keyFrames.emplace_back();
				AnimationKeyFrame& newKeyFrame = newTrack->m_keyFrames.back();
				newKeyFrame.m_time = time - static_cast<float>(startTick);

				newKeyFrame.m_time /= aiAnimation->mTicksPerSecond;

				newKeyFrame.m_position = prevPosition;
				newKeyFrame.m_scale = prevScale;
				newKeyFrame.m_rotation = prevRot;
			}
		}
	}
	AssetIO::Free(data);
}

AnimationTrack* Animation::createTrack(const std::string& name) {
	AnimationTrack* oldTrack = findTrack(name);
	if (oldTrack)
		return oldTrack;

	AnimationTrack& newTrack = m_tracks[name];
	newTrack.m_name = name;
	return &newTrack;
}

AnimationTrack* Animation::findTrack(const std::string& name_) const {
	auto it = m_tracks.find(name_);
	return it != m_tracks.end() ? const_cast<AnimationTrack*>(&(it->second)) : nullptr;
}

const std::string& Animation::getAnimationName() const {
	return m_animationName;
}

float Animation::getLength() const {
	return m_length;
}

const std::map<std::string, AnimationTrack>& Animation::getTracks() const {
	return m_tracks;
}

size_t Animation::getNumTracks() const {
	return m_tracks.size();
}

std::string& Animation::animationName() const {
	return m_animationName;
}

float& Animation::length() const {
	return m_length;
}

std::map<std::string, AnimationTrack>& Animation::tracks() const {
	return m_tracks;
}

void Animation::setPositionOfTrack(const std::string& name, float x, float y, float z) const {
	AnimationTrack* track = findTrack(name);
	if (track) {
		for (auto& keyFrame : track->m_keyFrames) {
			keyFrame.m_position.x = x;
			keyFrame.m_position.y = y;
			keyFrame.m_position.z = z;
		}
	}
}

void Animation::setScaleOfTrack(const std::string& name, float sx, float sy, float sz) const {
	AnimationTrack* track = findTrack(name);
	if (track) {
		for (auto& keyFrame : track->m_keyFrames) {
			keyFrame.m_scale.x = sx;
			keyFrame.m_scale.y = sy;
			keyFrame.m_scale.z = sz;
		}
	}
}

void Animation::scaleTrack(const std::string& name, float sx, float sy, float sz) const {
	AnimationTrack* track = findTrack(name);
	if (track) {
		for (auto& keyFrame : track->m_keyFrames) {
			keyFrame.m_scale.x *= sx;
			keyFrame.m_scale.y *= sy;
			keyFrame.m_scale.z *= sz;
		}
	}
}

void  Animation::shift(unsigned int ticks) {
	for (std::map<std::string, AnimationTrack>::iterator it = m_tracks.begin(); it != m_tracks.end();) {
		AnimationTrack& track = it->second;

		std::vector<AnimationKeyFrame> keyFrames;
		for (size_t frame = 0u; frame < track.m_keyFrames.size(); frame++) {
			size_t current = frame + ticks < track.m_keyFrames.size() ? frame + ticks :  (ticks + frame) - track.m_keyFrames.size();
			float time = track.m_keyFrames[frame].m_time;
			keyFrames.emplace_back();
			keyFrames.back().m_time = time;
			keyFrames.back().m_position = track.m_keyFrames[current].m_position;
			keyFrames.back().m_scale = track.m_keyFrames[current].m_scale;
			keyFrames.back().m_rotation = track.m_keyFrames[current].m_rotation;

		}
		track.m_keyFrames.assign(keyFrames.begin(), keyFrames.end());
		++it;
	}
}