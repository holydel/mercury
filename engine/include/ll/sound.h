#pragma once

#include "../mercury_api.h"

namespace mercury {
	namespace ll {
		namespace sound {
			struct SoundHandle : public Handle<u32>
			{
			};

			SoundHandle CreateSoundFromFile(const c8* filepath);
			void SoundPlay(SoundHandle sound);
			void SoundStop(SoundHandle sound);
			void SoundSetVolume(SoundHandle sound, float volume);
			void SoundSetPitch(SoundHandle sound, float pitch);
			void SoundSetPan(SoundHandle sound, float pan);
			void SoundSetLooping(SoundHandle sound, bool looping);
		}
	}
}