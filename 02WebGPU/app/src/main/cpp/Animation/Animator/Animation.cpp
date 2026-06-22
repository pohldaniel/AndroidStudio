#include <android/log.h>

#include <iostream>
#include "Animation.h"

Animation::Animation(ColladaLoader loader){
	loader.loadAnimation(m_keyFrames, m_duration, m_name);
}

/*std::string Animation::getName() {
	return m_name;
}

float Animation::getDuration() {

	return m_duration;
}*/