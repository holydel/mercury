#include "mercury_sound.h"

#ifdef MERCURY_LL_SOUND_NULL
#include "../mercury_ll_sound.h"

#include "mercury_log.h"
#include <vector>

using namespace mercury;
using namespace mercury::ll::sound;

struct MASound
{

};

std::vector<MASound> gAllMASounds;

void mercury::ll::sound::InitializeDevice()
{
	MLOG_DEBUG(u8"Initialize Sound System (NULL)");
}

void mercury::ll::sound::ShutdownDevice()
{
	MLOG_DEBUG(u8"Shutdown Sound System (NULL)");
}

void mercury::ll::sound::Tick()
{

}

SoundHandle mercury::ll::sound::CreateSoundFromFile(const c8* filepath)
{
	MASound& newSound = gAllMASounds.emplace_back();
	return SoundHandle{ static_cast<u32>(gAllMASounds.size() - 1) };
}

void mercury::ll::sound::SoundPlay(SoundHandle sound)
{
	MLOG_DEBUG(u8"PlaySound called (NULL) for sound handle %d", sound.handle);
}

#endif //MERCURY_LL_SOUND_NULL