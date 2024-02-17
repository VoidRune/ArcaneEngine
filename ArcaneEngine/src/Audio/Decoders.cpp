#include "Decoders.h"

#include "minivorbis.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include "minimp3_ex.h"

#include <iostream>

namespace Decoder
{
    static mp3dec_t s_Mp3d;

	void OggFromFile(const char* filename, std::vector<int16_t>& data, uint32_t& sampleRate, uint32_t& sizeInFrames, uint32_t& channels)
	{
        FILE* fp;
        fopen_s(&fp, filename, "rb");
        if (!fp) {
            std::cout << "audio file: " << filename << " could not be opened" << std::endl;
        }

        OggVorbis_File vorbis;
        if (ov_open_callbacks(fp, &vorbis, NULL, 0, OV_CALLBACKS_DEFAULT) != 0) {
            std::cout << "audio file: " << filename << " could not be opened" << std::endl;
            return;
        }

        vorbis_info* info = ov_info(&vorbis, -1);

        uint64_t samples = ov_pcm_total(&vorbis, -1);

        uint32_t sizeInBytes = sizeof(uint16_t) * info->channels * samples;

        uint8_t* buf = new uint8_t[sizeInBytes];
        uint8_t* bufferPtr = buf;

        while (true)
        {
            int section;
            long length = ov_read(&vorbis, (char*)bufferPtr, 4096, 0, 2, 1, &section);
            bufferPtr += length;
            if (length <= 0)
                break;
        }

        {
            data.resize(sizeInBytes / sizeof(int16_t));
            memcpy(data.data(), buf, sizeInBytes);
        }

        sizeInFrames = sizeInBytes / (sizeof(int16_t) * info->channels);
        sampleRate = info->rate;
        channels = info->channels;

        ov_clear(&vorbis);
    }

    void Mp3FromFile(const char* filename, std::vector<int16_t>& data, uint32_t& sampleRate, uint32_t& sizeInFrames, uint32_t& channels)
    {
        mp3dec_file_info_t info;
        if (mp3dec_load(&s_Mp3d, filename, &info, NULL, NULL))
        {
            std::cout << "audio file: " << filename << " could not be opened" << std::endl;
            return;
        }
        uint32_t sizeInBytes = info.samples * sizeof(mp3d_sample_t);

        {
            data.resize(sizeInBytes / sizeof(int16_t));
            memcpy(data.data(), info.buffer, sizeInBytes);
        }

        sizeInFrames = sizeInBytes / (sizeof(int16_t) * info.channels);
        sampleRate = info.hz;
        channels = info.channels;

        free(info.buffer);

    }

}