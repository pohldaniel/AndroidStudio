#pragma once
#include <AL/alc.h>

class SoundDevice{

public:
	static SoundDevice* get();
	static void Init();
	static void ShutDown();

private:

	SoundDevice();
	~SoundDevice();

	ALCdevice* m_alcDevice = nullptr;
	ALCcontext* m_alcContext = nullptr;

	static SoundDevice* Instance;
};

