#include "wav_parser.h"
#include <stdio.h>
#include <string.h>

// Convert 32-bit unsigned little-endian value to big-endian from byte array
static inline uint32_t little2big_u32(uint8_t const* data)
{
  return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

// Convert 16-bit unsigned little-endian value to big-endian from byte array
static inline uint16_t little2big_u16(uint8_t const* data)
{
  return data[0] | (data[1] << 8);
}

// Copy n bytes from source to destination and terminate the destination with
// null character. Destination must be at least (amount + 1) bytes big to
// account for null character.
static inline void bytes_to_string(uint8_t const* source, char* destination, size_t amount)
{
  memcpy(destination, source, amount);
  destination[amount] = '\0';
}

// Parse the header of WAV file and return WAVFile structure with header and
// pointer to data
WAVFile WAV_ParseFileData(uint8_t const* data)
{
  WAVFile file;
  uint8_t const* data_ptr = data;

  bytes_to_string(data_ptr, file.header.file_id, 4);
  data_ptr += 4;

  file.header.file_size = little2big_u32(data_ptr);
  data_ptr += 4;

  bytes_to_string(data_ptr, file.header.format, 4);
  data_ptr += 4;

  bytes_to_string(data_ptr, file.header.subchunk_id, 4);
  data_ptr += 4;

  file.header.subchunk_size = little2big_u32(data_ptr);
  data_ptr += 4;

  file.header.audio_format = little2big_u16(data_ptr);
  data_ptr += 2;

  file.header.number_of_channels = little2big_u16(data_ptr);
  data_ptr += 2;

  file.header.sample_rate = little2big_u32(data_ptr);
  data_ptr += 4;

  file.header.byte_rate = little2big_u32(data_ptr);
  data_ptr += 4;

  file.header.block_align = little2big_u16(data_ptr);
  data_ptr += 2;

  file.header.bits_per_sample = little2big_u16(data_ptr);
  data_ptr += 2;

  bytes_to_string(data_ptr, file.header.data_id, 4);
  data_ptr += 4;

  file.header.data_size = little2big_u32(data_ptr);
  data_ptr += 4;

  file.data = data_ptr;
  file.data_length = file.header.data_size;

  return file;
}

int _ReadSigned24BitInt(const std::uint8_t* dataPtr)
{
  // Combine the three bytes into a single integer using bit shifting and
  // masking. This works by isolating each byte using a bit mask (0xff) and then
  // shifting the byte to the correct position in the final integer.
  int value = dataPtr[0] | (dataPtr[1] << 8) | (dataPtr[2] << 16);

  // The value is stored in two's complement format, so if the most significant
  // bit (the 24th bit) is set, then the value is negative. In this case, we
  // need to extend the sign bit to get the correct negative value.
  if (value & (1 << 23))
  {
    value |= ~((1 << 24) - 1);
  }
  return value;
}

std::vector<float> WAV_ExtractSamples(const WAVFile wavFile) 
{
  std::vector<float> rawAudio = std::vector<float>();
  if (wavFile.header.audio_format == 3) {
    // IEEE 32-bit
    int bytePerSample = 4;
    int sampleCount = wavFile.data_length / bytePerSample;
    rawAudio.resize(sampleCount);
    for (auto i = 0; i < sampleCount; i++)
    {
      rawAudio[i] = (float)(((float*)wavFile.data)[i]);
    }
  }
  else if (wavFile.header.audio_format == 1) {
    // PCM
    if (wavFile.header.bits_per_sample == 16) {
      // 16-bit
      int sampleCount = wavFile.data_length / 2;
      rawAudio.resize(sampleCount);
      
      // Copy into the return array
      const float scale = 1.0 / ((double)(1 << 15));
      for (auto i = 0; i < sampleCount; i++)
      {
        rawAudio[i] = scale * (float)(((short*)wavFile.data)[i]); // 2^16
      }
    }
    else if (wavFile.header.bits_per_sample == 24) {
      // 24-bit
      int bytePerSample = 3;
      int sampleCount = wavFile.data_length / bytePerSample;

      // Copy into the return array
      const float scale = 1.0 / ((double)(1 << 23));
      rawAudio.resize(sampleCount);
      for (auto i = 0; i < sampleCount; i++)
      {
        rawAudio[i] = scale * ((float)_ReadSigned24BitInt(wavFile.data + i * bytePerSample));
      }
    }
    else if (wavFile.header.bits_per_sample == 32) {
      int bytePerSample = 4;
      int sampleCount = wavFile.data_length / bytePerSample;
      rawAudio.resize(sampleCount);
      for (auto i = 0; i < sampleCount; i++)
      {
        rawAudio[i] = (float)(((float*)wavFile.data)[i]);
      }
    }
  }
  return rawAudio;
}
