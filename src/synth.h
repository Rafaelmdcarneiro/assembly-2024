#ifndef _SYNTH_H
#define _SYNTH_H

#define AUDIO_SECONDS 110

/* outbuf passed to synth() must be at least this many bytes in size */
#define AUDIO_BUF_BYTES (3 * 44100 * AUDIO_SECONDS * 4)
/* audio to output is written starting from this position in the buffer */
#define START_PLAY_POS (1 * 44100 * AUDIO_SECONDS * 4)

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((regparm(2))) void synth(float *outbuf);

#ifdef __cplusplus
}
#endif

#endif /* _SYNTH_H */
