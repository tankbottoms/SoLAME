/* 
 * Audio 'LIB' defines
 */

#ifndef AUDIO_H_
#define AUDIO_H_

enum { AUDIO_OUT_HEADPHONES,AUDIO_OUT_INTERNAL_SPEAKER,AUDIO_OUT_LINE_OUT };

#define AUDIO_FORMAT_SIGNED_16    0x1
#define AUDIO_FORMAT_UNSIGNED_8   0x2
#define AUDIO_FORMAT_SIGNED_8     0x4
#define AUDIO_FORMAT_ULAW_8       0x8
#define AUDIO_FORMAT_ALAW_8       0x10

struct audio_info_struct
{
  long rate;
  int channels;
  int format;
  int bits;
};

#endif	// AUDIO_H_
