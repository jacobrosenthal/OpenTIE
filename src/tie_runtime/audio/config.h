#ifndef TIE_RUNTIME_AUDIO_CONFIG_H
#define TIE_RUNTIME_AUDIO_CONFIG_H

#include <stdbool.h>

#include "tie_runtime/runtime/profile_types.h"

typedef enum TieMidiBackendKind {
	TIE_MIDI_BACKEND_NONE,
	TIE_MIDI_BACKEND_FLUIDSYNTH,
	TIE_MIDI_BACKEND_FM4_OPL3,
	TIE_MIDI_BACKEND_SC55,
} TieMidiBackendKind;

/* Output speaker layouts. Non-stereo layouts mix positional flight SFX on a
 * quad front/rear bus; 5.1 and 7.1 route its rear pair to the back speakers
 * and leave centre, LFE and side channels silent. Music, speech and video
 * audio stay on the front pair. */
typedef enum TieSpeakerLayout {
	TIE_SPEAKER_LAYOUT_STEREO,
	TIE_SPEAKER_LAYOUT_QUAD,
	TIE_SPEAKER_LAYOUT_SURROUND_51,
	TIE_SPEAKER_LAYOUT_SURROUND_71,
} TieSpeakerLayout;

struct ImuseNukedSc55Romset;

typedef struct TieMidiBackendConfig {
	TieMidiBackendKind kind;
	const char* soundfont_path;
	const struct ImuseNukedSc55Romset* sc55_romset;
} TieMidiBackendConfig;

typedef struct TieAudioConfig {
	TieMidiBackendConfig midi_backend;
	bool sb16_filter_enabled;
	bool prefer_tie95_frontend_voices;
	TieMusicSource music_source;
	int music_ducking_volume_percent;
	TieSpeakerLayout speaker_layout;
} TieAudioConfig;

bool TieMidiBackend_Available(TieMidiBackendKind kind);
void TieAudio_Configure(const TieAudioConfig* config);
const TieAudioConfig* TieAudio_Config(void);
bool TieAudio_SetMusicDuckingVolumePercent(int percent);
bool TieSpeakerLayout_Valid(TieSpeakerLayout layout);
/* Interleaved device channel count in SDL order (2, 4, 6 or 8). */
int TieSpeakerLayout_Channels(TieSpeakerLayout layout);
/* True when positional sounds are mixed front/rear instead of mirrored. */
bool TieAudio_SurroundEnabled(void);

#endif
