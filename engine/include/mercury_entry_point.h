#pragma once
#include "mercury_api.h"

#ifdef MERCURY_LL_OS_ANDROID
#include <jni.h>
#include <android/native_activity.h>

void DummyLinkerExportFunctionForAndroidMain()
{
	ANativeActivity* activity = nullptr;
	void* savedState = nullptr;
	size_t savedStateSize = 0;
	ANativeActivity_onCreate(activity, savedState, savedStateSize);
}

#endif