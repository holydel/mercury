export module EditorState;

import Project;

bool gRunning = true;

export namespace EditorState {
	export bool IsRunning() { return gRunning; }
	export void SetRunning(bool running) { gRunning = running; }
	export MProject gCurrentProject;
}