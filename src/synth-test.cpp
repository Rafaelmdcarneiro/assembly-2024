#include "dnload.h"

#include "synth.h"

/// Sampling rate.
#define AUDIO_SAMPLERATE 44100

/// Channels.
#define AUDIO_CHANNELS 2

/// Sample size.
#define AUDIO_SAMPLE_SIZE 4

/// Seconds to write.
#define SECONDS 109

/// Output file to write.
#define SYNTH_TEST_OUTPUT_FILE "olkiluoto_3-2-1.wav"

/// Output buffer.
static uint8_t g_scratch_buffer[AUDIO_BUF_BYTES * 9 / 8];

/// Offset into the output buffer where the audio actually starts.
static uint8_t* g_audio_buffer = g_scratch_buffer + START_PLAY_POS;

/// Entry point.
void _start()
{
    dnload();

    synth(reinterpret_cast<float*>(g_scratch_buffer));

    // Write raw .wav file.
    {
        SF_INFO info;
        info.samplerate = AUDIO_SAMPLERATE;
        info.channels = AUDIO_CHANNELS;
        info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

        SNDFILE *outfile = dnload_sf_open(SYNTH_TEST_OUTPUT_FILE, SFM_WRITE, &info);
        sf_count_t write_count = AUDIO_SAMPLERATE * SECONDS;
        dnload_sf_writef_float(outfile, reinterpret_cast<float*>(g_audio_buffer), write_count);
        dnload_sf_close(outfile);
    }

    asm_exit();
}

