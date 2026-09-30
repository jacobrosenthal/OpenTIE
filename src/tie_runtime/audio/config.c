#include "tie_runtime/audio/imuse_session.h"

static TieAudioConfig s_audio_config;

void TieAudio_Configure(const TieAudioConfig* config) {
	s_audio_config = config ? *config : (TieAudioConfig) { 0 };
}

const TieAudioConfig* TieAudio_Config(void) { return &s_audio_config; }

bool TieSpeakerLayout_Valid(TieSpeakerLayout layout) {
	return layout == TIE_SPEAKER_LAYOUT_STEREO || layout == TIE_SPEAKER_LAYOUT_QUAD ||
		   layout == TIE_SPEAKER_LAYOUT_SURROUND_51 || layout == TIE_SPEAKER_LAYOUT_SURROUND_71;
}

int TieSpeakerLayout_Channels(TieSpeakerLayout layout) {
	switch (layout) {
		case TIE_SPEAKER_LAYOUT_QUAD:
			return 4;
		case TIE_SPEAKER_LAYOUT_SURROUND_51:
			return 6;
		case TIE_SPEAKER_LAYOUT_SURROUND_71:
			return 8;
		case TIE_SPEAKER_LAYOUT_STEREO:
			break;
	}
	return 2;
}

bool TieAudio_SurroundEnabled(void) { return s_audio_config.speaker_layout != TIE_SPEAKER_LAYOUT_STEREO; }

void TieAudio_SetSpeakerLayout(TieSpeakerLayout layout) {
	if (TieSpeakerLayout_Valid(layout))
		s_audio_config.speaker_layout = layout;
}

bool TieAudio_SetMusicDuckingVolumePercent(int percent) {
	if ((unsigned int)percent > 100u)
		return false;
	s_audio_config.music_ducking_volume_percent = percent;
	return true;
}
