#ifndef OPTIONS_H_
#define OPTIONS_H_

//#define PROFILE
//#define PROFILE_NODEBUG
//#define DEBUG

//#define MEASURE_TIME 2

#define BUFFER_SIZE	48	// Size of buffer, in kilobytes.
//#define FORCE_FREQUENCY	11000	// Force the clip to be played at the specified frequency.

//#define SKIP_START_FRAMES	30	// How many frames should be skipped at the start?
//#define SKIP_N_FRAMES	2	// Play only every nth frame.

// One DOWNSAMPLE value must be defined.
//Now defined in g_DownSampleRate (MP3DLL.CPP)
//#define DOWNSAMPLE	0	// No downsampling.
//#define DOWNSAMPLE	1	// 1:2 downsampling.
//#define DOWNSAMPLE	2	// 1:4 downsampling.

// If none of the following are defined, then both channels are decoded.
//#define DECODE_CHANNEL	0	// Left channel only.
//#define DECODE_CHANNEL	1	// Right channel only.
//#define DECODE_CHANNEL	3	// Force mono mix.

//#define TRY_RESYNCH	// Resync on errors.

// If none of the following are defined, then the program runs in test mode with no output.
#define OUTPUT_AUDIO		// Send the output to the audio device.
//#define OUTPUT_STDOUT		// Send the output to stdout.
//#define OUTPUT_BUFFER		// Send the output to a buffer.

#define FORCE_8_BIT	0	// Don't force 8 bit mode.
//#define FORCE_8_BIT	1	// Force 8 bit mode.

// Define the output scale (volume?).  The default is 32768.
// This should really be a const long, but we'll fix that later.
#define OUTSCALE	32768


#endif /* OPTIONS_H_ */