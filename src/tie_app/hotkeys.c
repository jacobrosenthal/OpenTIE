#include "tie_app/hotkeys.h"

#include "aeron/aeron.h"
#include "tie_app/settings/audio_options.h"
#include "tie_app/settings/settings.h"
#include "tie_app/settings/video_options.h"
#include "tie_runtime/input/input.h"
#include "tie_runtime/input/keyboard_mapping.h"
#include "tie_runtime/snapshot/snapshot.h"

static void TieHotkeys_ReconcileFullscreen(TieHotkeys* hotkeys) {
	const int fullscreen = Aeron_Fullscreen();
	if (fullscreen == hotkeys->last_fullscreen)
		return;

	TieAppVideoConfig options;
	char error[256];
	TieVideoOptions_Get(&options);
	options.fullscreen = fullscreen != 0;
	if (!TieVideoOptions_Set(&options, error, sizeof error))
		Aeron_LogWarn("tie", "could not reconcile fullscreen mode: %s", error);
	hotkeys->last_fullscreen = fullscreen;
}

static int TieHotkeys_Trigger(const AeronInputSnapshot* input, TieKeyboardShortcut shortcut) {
	if (!input || !input->has_focus || input->key_events_overflow)
		return -1;
	for (uint16_t i = 0; i < input->key_event_count; ++i) {
		const AeronKeyEvent* event = &input->key_events[i];
		if (event->down && !event->repeat && TieKeyboardMapping_Shortcut(event->chord) == shortcut)
			return event->chord.key;
	}
	return -1;
}

static void TieHotkeys_ProcessDebugUi(const AeronInputSnapshot* input) {
	const int trigger = TieHotkeys_Trigger(input, TIE_KEYBOARD_SHORTCUT_DEBUG);
	if (trigger < 0)
		return;
	Aeron_DebugUiToggle();
	TieInput_SuppressKey(trigger);
}

static void TieHotkeys_ProcessFullscreen(const AeronInputSnapshot* input) {
	const int trigger = TieHotkeys_Trigger(input, TIE_KEYBOARD_SHORTCUT_FULLSCREEN);
	if (trigger < 0)
		return;

	TieAppVideoConfig options;
	char error[256];
	TieVideoOptions_Get(&options);
	options.fullscreen = !Aeron_Fullscreen();
	if (!TieVideoOptions_Set(&options, error, sizeof error))
		Aeron_LogWarn("tie", "could not toggle fullscreen mode: %s", error);
	TieInput_SuppressKey(trigger);
}

/* Ctrl+Alt+S switches between stereo and the last surround layout (5.1 when
 * none has been used). The choice is saved like the settings selector. */
static void TieHotkeys_ProcessSurround(TieHotkeys* hotkeys, const AeronInputSnapshot* input) {
	const int trigger = TieHotkeys_Trigger(input, TIE_KEYBOARD_SHORTCUT_SURROUND);
	if (trigger < 0)
		return;
	TieInput_SuppressKey(trigger);

	TieAppLiveAudioOptions options;
	char error[256];
	TieAudioOptions_Get(&options);
	if (options.speaker_layout != TIE_SPEAKER_LAYOUT_STEREO) {
		hotkeys->surround_layout = options.speaker_layout;
		options.speaker_layout = TIE_SPEAKER_LAYOUT_STEREO;
	} else {
		options.speaker_layout = hotkeys->surround_layout;
	}
	if (!TieAudioOptions_Set(&options, error, sizeof error)) {
		Aeron_LogWarn("tie.audio", "could not toggle surround sound: %s", error);
		return;
	}
	Aeron_LogInfo("tie.audio", "surround sound %s (%d channels)",
				  options.speaker_layout == TIE_SPEAKER_LAYOUT_STEREO ? "disabled" : "enabled",
				  TieSpeakerLayout_Channels(options.speaker_layout));
}

static bool TieHotkeys_ControllerStartPressed(const AeronInputSnapshot* input) {
	if (!input)
		return false;
	for (int pad = 0; pad < AERON_CONTROLLER_MAX; ++pad) {
		if (input->controllers[pad].connected &&
			(input->controllers[pad].gamepad_pressed_buttons & (1u << AERON_GAMEPAD_BUTTON_START)))
			return true;
	}
	return false;
}

static bool TieHotkeys_ProcessSettings(const AeronInputSnapshot* input) {
	const bool was_open = TieSettings_Open();
	if (input && input->has_focus && TieSettings_Available()) {
		const TieSnapshot* snapshot = TieSnapshot_Current();
		const int escape = TieHotkeys_Trigger(input, TIE_KEYBOARD_SHORTCUT_SETTINGS);
		/* Outside flight, the frontend owns Escape through the raw key queue. */
		if (escape >= 0 && !was_open && snapshot && snapshot->scene_kind == TIE_SCENE_FLIGHT) {
			TieInput_SuppressKey(escape);
			TieSettings_Show();
			return true;
		}
		const bool start_pressed = TieHotkeys_ControllerStartPressed(input);
		const bool start_controls_settings =
			was_open || !snapshot || snapshot->scene_kind != TIE_SCENE_FLIGHT;
		if (start_pressed && start_controls_settings && !TieSettings_CapturesController() &&
			!TieSettings_CapturesKeyboard())
			TieSettings_Toggle();
	}
	return was_open || TieSettings_Open();
}

static void TieHotkeys_ProcessPause(TieHotkeys* hotkeys, const AeronInputSnapshot* input) {
	const int trigger = TieHotkeys_Trigger(input, TIE_KEYBOARD_SHORTCUT_PAUSE);
	if (trigger < 0)
		return;
	hotkeys->paused = !hotkeys->paused;
	TieInput_ResetThrottle();
	TieInput_SuppressKey(trigger);
}

void TieHotkeys_Init(TieHotkeys* hotkeys) {
	if (!hotkeys)
		return;
	hotkeys->last_fullscreen = Aeron_Fullscreen();
	hotkeys->paused = false;
	hotkeys->surround_layout = TIE_SPEAKER_LAYOUT_SURROUND_51;
	TieAppLiveAudioOptions audio = { 0 };
	TieAudioOptions_Get(&audio);
	if (audio.speaker_layout != TIE_SPEAKER_LAYOUT_STEREO && TieSpeakerLayout_Valid(audio.speaker_layout))
		hotkeys->surround_layout = audio.speaker_layout;
}

TieHotkeysFrame TieHotkeys_Process(TieHotkeys* hotkeys, const AeronInputSnapshot* input) {
	TieHotkeysFrame frame = { 0 };
	if (!hotkeys)
		return frame;

	TieHotkeys_ReconcileFullscreen(hotkeys);
	if (!TieSettings_CapturesKeyboard()) {
		TieHotkeys_ProcessDebugUi(input);
		TieHotkeys_ProcessFullscreen(input);
		TieHotkeys_ProcessSurround(hotkeys, input);
	}
	const bool was_open = TieSettings_Open();
	frame.menu_open = TieHotkeys_ProcessSettings(input);
	frame.settings_opened = !was_open && TieSettings_Open();
	if (!TieSettings_CapturesKeyboard())
		TieHotkeys_ProcessPause(hotkeys, input);
	frame.paused = hotkeys->paused;
	return frame;
}
