#include "tie_app/settings/audio_options.h"

#include <string.h>

static struct {
	TieAppLiveAudioOptions persisted;
	TieAppLiveAudioOptions requested;
	TieAudioOptionsApplyFn apply;
	TieAudioOptionsPersistFn persist;
	void* user;
	bool configured;
	bool dirty;
} g_audio_options;

static bool TieAudioOptions_Valid(const TieAppLiveAudioOptions* options) {
	return options && (unsigned int)options->music_ducking_volume_percent <= 100u &&
		   TieSpeakerLayout_Valid(options->speaker_layout);
}

static bool TieAudioOptions_Equal(const TieAppLiveAudioOptions* left, const TieAppLiveAudioOptions* right) {
	return left->music_ducking_volume_percent == right->music_ducking_volume_percent &&
		   left->speaker_layout == right->speaker_layout;
}

bool TieAudioOptions_Configure(const TieAppLiveAudioOptions* requested, TieAudioOptionsApplyFn apply,
							   TieAudioOptionsPersistFn persist, void* user) {
	memset(&g_audio_options, 0, sizeof g_audio_options);
	if (!TieAudioOptions_Valid(requested) || !apply || !persist)
		return false;
	g_audio_options.persisted = *requested;
	g_audio_options.requested = *requested;
	g_audio_options.apply = apply;
	g_audio_options.persist = persist;
	g_audio_options.user = user;
	g_audio_options.configured = true;
	return true;
}

void TieAudioOptions_Shutdown(void) { memset(&g_audio_options, 0, sizeof g_audio_options); }

void TieAudioOptions_Get(TieAppLiveAudioOptions* out) {
	if (out && g_audio_options.configured)
		*out = g_audio_options.requested;
}

bool TieAudioOptions_Set(const TieAppLiveAudioOptions* options, char* error, size_t error_capacity) {
	if (!g_audio_options.configured || !TieAudioOptions_Valid(options))
		return false;
	if (TieAudioOptions_Equal(options, &g_audio_options.requested))
		return true;
	if (!g_audio_options.apply(&g_audio_options.requested, options, g_audio_options.user, error,
							   error_capacity))
		return false;
	g_audio_options.requested = *options;
	g_audio_options.dirty = !TieAudioOptions_Equal(&g_audio_options.requested, &g_audio_options.persisted);
	return true;
}

bool TieAudioOptions_Flush(char* error, size_t error_capacity) {
	if (!g_audio_options.configured || !g_audio_options.dirty)
		return true;
	if (!g_audio_options.persist(&g_audio_options.requested, g_audio_options.user, error, error_capacity))
		return false;
	g_audio_options.persisted = g_audio_options.requested;
	g_audio_options.dirty = false;
	return true;
}
