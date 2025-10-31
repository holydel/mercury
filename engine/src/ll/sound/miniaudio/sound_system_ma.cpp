#include "mercury_sound.h"

#ifdef MERCURY_LL_SOUND_MINIAUDIO
#include "../mercury_ll_sound.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "mercury_log.h"
#include <vector>

using namespace mercury;
using namespace mercury::ll::sound;

ma_engine gMAEngine;

struct MASound
{
	ma_sound sound;
};

std::vector<MASound> gAllMASounds;

void MercurySoundInitialize()
{
	MLOG_DEBUG(u8"Initialize Sound System (MINIAUDIO)");

	ma_engine_config gEngineConfig = ma_engine_config_init();

	ma_result res = ma_engine_init(&gEngineConfig, &gMAEngine);
	if (res != MA_SUCCESS)
	{
		MLOG_ERROR(u8"ma_engine_init failed (err=%d)", (int)res);
		return;
	}
}

void MercurySoundShutdown()
{
	MLOG_DEBUG(u8"Shutdown Sound System (MINIAUDIO)");

	ma_engine_uninit(&gMAEngine);
}

void MercurySoundTick()
{

}


SoundHandle mercury::ll::sound::CreateSoundFromFile(const c8* filepath)
{
	MLOG_DEBUG(u8"CreateSoundFromFile (MINIAUDIO): %s", filepath ? filepath : MSTR("<null>"));

	// Add a new container first (note: vector reallocation may move items; consider using a non-moving container if needed).	
	MASound& newSound = gAllMASounds.emplace_back();

	// miniaudio expects const char*, convert from c8* (char8_t) safely.
	const char* path = reinterpret_cast<const char*>(filepath);

	ma_result res = ma_sound_init_from_file(&gMAEngine, path, MA_SOUND_FLAG_LOOPING | MA_SOUND_FLAG_STREAM, nullptr, nullptr, &newSound.sound);
	if (res != MA_SUCCESS)
	{
		MLOG_ERROR(u8"ma_sound_init_from_file failed for '%s' (err=%d)", filepath ? filepath : MSTR("<null>"), (int)res);
		gAllMASounds.pop_back();
		return SoundHandle{}; // invalid handle
	}

	return SoundHandle{ static_cast<u32>(gAllMASounds.size() - 1) };

}

void mercury::ll::sound::SoundPlay(SoundHandle sound)
{
	MLOG_DEBUG(u8"PlaySound called (MINIAUDIO) for sound handle %d", sound.handle);

	if (!sound.isValid() || sound.handle >= gAllMASounds.size())
	{
		MLOG_ERROR(u8"SoundPlay: Invalid sound handle %d", sound.handle);
		return;
	}

	ma_sound_start(&gAllMASounds[sound.handle].sound);
}

void mercury::ll::sound::SoundStop(SoundHandle sound)
{
	ma_sound_stop(&gAllMASounds[sound.handle].sound);
}

void mercury::ll::sound::SoundSetVolume(SoundHandle sound, float volume)
{
	ma_sound_set_volume(&gAllMASounds[sound.handle].sound, volume);
}

void mercury::ll::sound::SoundSetPitch(SoundHandle sound, float pitch)
{
	ma_sound_set_pitch(&gAllMASounds[sound.handle].sound, pitch);

}

void mercury::ll::sound::SoundSetPan(SoundHandle sound, float pan)
{
	ma_sound_set_pan(&gAllMASounds[sound.handle].sound, pan);
}

void mercury::ll::sound::SoundSetLooping(SoundHandle sound, bool looping)
{
	ma_sound_set_looping(&gAllMASounds[sound.handle].sound, looping);
}

#endif //MERCURY_LL_SOUND_MINIAUDIO