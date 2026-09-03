#include <vector>
#include <iostream>
#include <AL/al.h>
#include "SoundDevice.h"

SoundDevice* SoundDevice::Instance = nullptr;

SoundDevice* SoundDevice::get(){
	Init();
	return Instance;
}

void SoundDevice::Init(){
	if (Instance == nullptr)
		Instance = new SoundDevice();
}

void SoundDevice::ShutDown() {
	delete Instance;
}

SoundDevice::SoundDevice(){
	m_alcDevice = alcOpenDevice(nullptr);
	if (!m_alcDevice)
		throw("failed to get sound device");

    if (alcIsExtensionPresent(m_alcDevice, "ALC_SOFT_HRTF")) {

    }

	m_alcContext = alcCreateContext(m_alcDevice, nullptr);
	if (!m_alcContext)
		throw("failed to set sound context");

	if (!alcMakeContextCurrent(m_alcContext))
		throw("failed to make context current");

}

SoundDevice::~SoundDevice(){
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(m_alcContext);
	alcCloseDevice(m_alcDevice);
}
