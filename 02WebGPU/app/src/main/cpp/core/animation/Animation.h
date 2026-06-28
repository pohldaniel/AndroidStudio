#pragma once

#include <string>
#include <vector>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

static const unsigned char CHANNEL_POSITION = 1;
static const unsigned char CHANNEL_ROTATION = 2;
static const unsigned char CHANNEL_SCALE = 4;

struct AnimationKeyFrame {

	AnimationKeyFrame() : m_time(0.0f), m_position(glm::vec3(0.0f, 0.0f, 0.0f)), m_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), m_scale(glm::vec3(1.0f, 1.0f, 1.0f)) {
	}

	float m_time;
	glm::vec3 m_position;
	glm::quat m_rotation;
	glm::vec3 m_scale;
};

struct AnimationTrack {

	void findKeyFrameIndex(float time, size_t& index) const;
	std::string m_name;
	unsigned char m_channelMask;
	std::vector<AnimationKeyFrame> m_keyFrames;
};

class Animation {

	friend class AnimatedModel;


public:

	Animation();
	~Animation();

	void loadAnimationAssimp(const std::string& filename, std::string sourceName, std::string destName);
	void loadAnimation(std::string filename);

	AnimationTrack* createTrack(const std::string& name);
	AnimationTrack* findTrack(const std::string& name) const;

	const std::string& getAnimationName() const;
	float getLength() const;
	const std::map<std::string, AnimationTrack>& getTracks() const;
	const std::vector<AnimationTrack> getAnimationTracks() const;
	size_t getNumTracks() const;

	std::string& animationName() const;
	float& length() const;
	std::map<std::string, AnimationTrack>& tracks() const;

	void setPositionOfTrack(const std::string& name, const float x, const float y, const float z);
	void setScaleOfTrack(const std::string& name, const float sx, const float sy, const float sz);
	void scaleTrack(const std::string& name, const float sx, const float sy, const float sz);

private:

	mutable std::string m_animationName;
	mutable float m_length;
	mutable std::map<std::string, AnimationTrack> m_tracks;
};