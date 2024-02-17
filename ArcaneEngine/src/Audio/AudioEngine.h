#pragma once
#include <vector>
#include <string>

namespace Arc
{
	class AudioSource
	{
	private:
		bool m_Loaded = false;
		uint32_t m_SampleRate;
		uint32_t m_SizeInFrames;
		uint32_t m_Channels;
		std::vector<int16_t> m_Data;

		friend class AudioEngine;
	};

	class SoundInstance
	{
	private:
		uint32_t m_SoundIndex = 0;
		uint32_t m_SoundHash = 0;

		friend class AudioEngine;
	};

	class SoundGroup
	{
	private:
		int32_t m_SoundGroupIndex = -1;

		friend class AudioEngine;
	};

	class AudioEngine
	{
	public:
		AudioEngine(uint32_t maxConcurrentSounds = 64);
		~AudioEngine();

		SoundInstance Play(AudioSource* audioSource, SoundGroup* soundGroup = nullptr);
		void Stop(SoundInstance* soundInstance);

		void SetSpatialization(SoundInstance* soundInstance, float x, float y);
		void SetLooping(SoundInstance* soundInstance, bool looping);

		void SeekToTime(SoundInstance* soundInstance, float timeInSeconds);
		void SeekToFrame(SoundInstance* soundInstance, uint32_t frame);

		void SetMasterVolume(float volume);
		void SetVolume(SoundGroup* soundGroup, float volume);
		void SetVolume(SoundInstance* soundInstance, float volume);

		void CreateSoundGroup(SoundGroup* soundGroup, float volume = 1.0f);
		void LoadFromFile(AudioSource* sound, const std::string& filename);
	private:

	};
}