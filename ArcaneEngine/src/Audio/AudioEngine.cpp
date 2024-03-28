#include "AudioEngine.h"

/* Exclude unnecessary symbols */
#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#define NOGDI

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#include "miniaudio.h"

#include "Decoders.h"
#include "Core/Log.h"

#include <iostream>
#include <filesystem>
#include <list>

namespace Arc
{
    ma_context s_Context;
    ma_device s_PlaybackDevice;

    struct SoundInstanceInternal
    {
        uint32_t FrameIndex = 0;
        uint32_t SizeInFrames = 0;
        uint32_t SampleRate = 0;
        uint32_t Channels = 0;
        float Volume = 1.0f;
        int32_t SoundGroupIndex = -1;
        float Left = 1.0f;
        float Right = 1.0f;
        bool Looping = false;
        std::vector<int16_t>* pData = nullptr;
        uint32_t Hash = 0;
    };

    std::vector<uint32_t> s_IndexStack;
    std::vector<SoundInstanceInternal> s_SoundPool;

    std::list<uint32_t> s_ActiveSounds;
    uint32_t s_GlobalHash = 0;
    std::vector<float> s_SoundGroupPool;

    void audio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
    {
        std::list<uint32_t>::iterator index = s_ActiveSounds.begin();

        while (index != s_ActiveSounds.end()) 
        {
            SoundInstanceInternal& sound = s_SoundPool[*index];
            uint32_t frames = std::min(frameCount, sound.SizeInFrames - sound.FrameIndex);
            if (frames <= 0)
            {
                if (sound.Looping)
                {
                    sound.FrameIndex = 0;
                    frames = std::min(frameCount, sound.SizeInFrames - sound.FrameIndex);
                }
                else
                {
                    s_IndexStack.push_back(*index);
                    index = s_ActiveSounds.erase(index);
                    continue;
                }
            }
            float v = sound.Volume * ((sound.SoundGroupIndex == -1) ? 1.0f : s_SoundGroupPool[sound.SoundGroupIndex]);
            float vl = v * sound.Left;
            float vr = v * sound.Right;
            for (size_t i = 0; i < frames; i++)
            {
                // Left
                ((float*)pOutput)[i * 2 + 0] += vl * (*sound.pData)[sound.Channels * (sound.FrameIndex + i) + 0] / 32767.0f;
                // Right
                ((float*)pOutput)[i * 2 + 1] += vr * (*sound.pData)[sound.Channels * (sound.FrameIndex + i) + sound.Channels - 1] / 32767.0f;
            }
            sound.FrameIndex += frames;
        
            index++;
        }
    }
    
    void AudioEngine::Initialize(uint32_t maxConcurrentSounds)
    {
        if (ma_context_init(NULL, 0, NULL, &s_Context) != MA_SUCCESS) {
            ARC_LOG("Failed to initialize audio context!");
            return;
        }

        {
            ma_device_info* pPlaybackDeviceInfos;
            ma_uint32 playbackDeviceCount;
            if (ma_context_get_devices(&s_Context, &pPlaybackDeviceInfos, &playbackDeviceCount, nullptr, nullptr) == MA_SUCCESS) {

                for (int iDevice = 0; iDevice < playbackDeviceCount; iDevice++) {
                    ma_device_info* deviceInfo = &pPlaybackDeviceInfos[iDevice];
            
                    if (deviceInfo->isDefault == true)
                    {
                        ARC_LOG(std::string("Using playback device: ") + deviceInfo->name);
                    }
                }
            }
        }

        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.capture.pDeviceID = nullptr;
        deviceConfig.capture.format = ma_format_f32;
        deviceConfig.capture.channels = 2;
        deviceConfig.capture.shareMode = ma_share_mode_shared;
        deviceConfig.playback.pDeviceID = nullptr;
        deviceConfig.playback.format = ma_format_f32;
        deviceConfig.playback.channels = 2;
        deviceConfig.sampleRate = 44100;
        deviceConfig.dataCallback = audio_data_callback;
        deviceConfig.pUserData = nullptr;

        if (ma_device_init(&s_Context, &deviceConfig, &s_PlaybackDevice) != MA_SUCCESS) {
            ARC_LOG("Failed to initialize playback device!");
            return;
        }

        ma_device_start(&s_PlaybackDevice);

        s_SoundPool.resize(maxConcurrentSounds);
        for (size_t i = 0; i < s_SoundPool.size(); i++)
        {
            s_IndexStack.push_back(s_SoundPool.size() - i - 1);
        }
    }

    void AudioEngine::Shutdown()
    {
        ma_device_uninit(&s_PlaybackDevice);
        ma_context_uninit(&s_Context);
    }

    void AudioEngine::SetSpatialization(SoundInstance* soundInstance, float x, float y)
    {
        if (soundInstance->m_SoundHash != s_SoundPool[soundInstance->m_SoundIndex].Hash)
            return;

        float sn = x * x + y * y;
        sn = std::max(sn, 0.5f);

        float c = (x * x) / sn;
        c *= (0 < x) - (x < 0);

        double leftEarLevel = 0.5 * (1.0 - c) / sn;
        double rightEarLevel = 0.5 * (1.0 + c) / sn;

        s_SoundPool[soundInstance->m_SoundIndex].Left = leftEarLevel;
        s_SoundPool[soundInstance->m_SoundIndex].Right = rightEarLevel;
    }

    void AudioEngine::SetLooping(SoundInstance* soundInstance, bool looping)
    {
        if (soundInstance->m_SoundHash != s_SoundPool[soundInstance->m_SoundIndex].Hash)
            return;

        s_SoundPool[soundInstance->m_SoundIndex].Looping = looping;
    }

    void AudioEngine::SeekToTime(SoundInstance* soundInstance, float timeInSeconds)
    {
        if (soundInstance->m_SoundHash != s_SoundPool[soundInstance->m_SoundIndex].Hash)
            return;

        s_SoundPool[soundInstance->m_SoundIndex].FrameIndex = timeInSeconds * s_SoundPool[soundInstance->m_SoundIndex].SampleRate;
    }

    void AudioEngine::SeekToFrame(SoundInstance* soundInstance, uint32_t frame)
    {
        if (soundInstance->m_SoundHash != s_SoundPool[soundInstance->m_SoundIndex].Hash)
            return;

        s_SoundPool[soundInstance->m_SoundIndex].FrameIndex = frame;
    }

    SoundInstance AudioEngine::Play(AudioSource* audioSource, SoundGroup* soundGroup)
    {
        if (audioSource->m_Loaded == false || 
            s_IndexStack.size() == 0)
            return {};

        uint32_t index = s_IndexStack[s_IndexStack.size() - 1];
        s_IndexStack.pop_back();

        SoundInstanceInternal& soundInstance = s_SoundPool[index];

        soundInstance.FrameIndex = 0;
        soundInstance.SizeInFrames = audioSource->m_SizeInFrames;
        soundInstance.SampleRate = audioSource->m_SampleRate;
        soundInstance.Channels = audioSource->m_Channels;
        soundInstance.Volume = 1.0f;
        soundInstance.SoundGroupIndex = (soundGroup == nullptr) ? -1 : soundGroup->m_SoundGroupIndex;
        soundInstance.Left = 1.0f * soundInstance.Volume;
        soundInstance.Right = 1.0f * soundInstance.Volume;
        soundInstance.Looping = false;
        soundInstance.pData = &audioSource->m_Data;

        s_ActiveSounds.push_back(index);

        SoundInstance inst;
        inst.m_SoundIndex = index;
        inst.m_SoundHash = s_GlobalHash;
        return inst;
    }

    void AudioEngine::Stop(SoundInstance* soundInstance)
    {
        if (soundInstance->m_SoundHash != s_SoundPool[soundInstance->m_SoundIndex].Hash)
            return;

        s_SoundPool[soundInstance->m_SoundIndex].FrameIndex = s_SoundPool[soundInstance->m_SoundIndex].SizeInFrames;
        s_SoundPool[soundInstance->m_SoundIndex].Looping = false;
    }

    void AudioEngine::SetMasterVolume(float volume)
    {
        ma_device_set_master_volume(&s_PlaybackDevice, std::clamp(volume, 0.0f, 1.0f));
    }

    void AudioEngine::SetVolume(SoundGroup* soundGroup, float volume)
    {
        if (soundGroup->m_SoundGroupIndex == -1)
            return;
        s_SoundGroupPool[soundGroup->m_SoundGroupIndex] = std::clamp(volume, 0.0f, 1.0f);
    }

    void AudioEngine::SetVolume(SoundInstance* soundInstance, float volume)
    {
        if (soundInstance->m_SoundHash != s_SoundPool[soundInstance->m_SoundIndex].Hash)
            return;

        s_SoundPool[soundInstance->m_SoundIndex].Volume = std::clamp(volume, 0.0f, 1.0f);
    }

    void AudioEngine::CreateSoundGroup(SoundGroup* soundGroup, float volume)
    {
        soundGroup->m_SoundGroupIndex = s_SoundGroupPool.size();
        s_SoundGroupPool.push_back(volume);
    }

    void AudioEngine::LoadFromFile(AudioSource* sound, const std::string& filename)
    {
        std::filesystem::path path = filename;
        std::string extension = path.extension().string();

        if (extension == ".mp3")  Decoder::Mp3FromFile(filename.c_str(), sound->m_Data, sound->m_SampleRate, sound->m_SizeInFrames, sound->m_Channels);
        if (extension == ".ogg")  Decoder::OggFromFile(filename.c_str(), sound->m_Data, sound->m_SampleRate, sound->m_SizeInFrames, sound->m_Channels);

        sound->m_Loaded = true;
    }
}