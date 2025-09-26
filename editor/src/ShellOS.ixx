module;
#include <mercury_api.h>
#include <string>
#include <filesystem>

#ifdef MERCURY_LL_OS_WIN32
#include <windows.h>
#include <shlobj.h>
#endif

export module ShellOS;

#ifdef MERCURY_LL_OS_WIN32
export namespace ShellOS {
	export std::filesystem::path GetUserHomeDirectory()
	{
		wchar_t path[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path))) {
			return std::filesystem::path(path);
		}
		return std::filesystem::path();
	}

	export std::filesystem::path GetUserDocumentsDirectory()
	{
		wchar_t path[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
			return std::filesystem::path(path);
		}
		return std::filesystem::path();
	}

	export std::filesystem::path SelectFolderDialog(const std::filesystem::path& initialPath) {
		BROWSEINFOW bi = { 0 };
		bi.hwndOwner = GetForegroundWindow(); // Set owner to current active window
		bi.lpszTitle = L"Select Folder";
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		
		// Set initial directory if provided
		if (!initialPath.empty()) {
			bi.lpfn = [](HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) -> int {
				if (uMsg == BFFM_INITIALIZED && lpData) {
					SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, lpData);
				}
				return 0;
			};
			bi.lParam = (LPARAM)initialPath.c_str();
		}
		
		LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
		if (pidl != NULL) {
			wchar_t path[MAX_PATH];
			if (SHGetPathFromIDListW(pidl, path)) {
				CoTaskMemFree(pidl);
				return std::filesystem::path(path);
			}
			CoTaskMemFree(pidl);
		}
		return std::filesystem::path();
	}

	export std::filesystem::path SelectFileDialog(const std::filesystem::path& initialPath, const char* filter) {
		OPENFILENAMEW ofn;       // common dialog box structure
		wchar_t szFile[MAX_PATH] = { 0 };       // buffer for file name
		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = GetForegroundWindow(); // Set owner to current active window
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrTitle = L"Select File";
		ofn.lpstrInitialDir = initialPath.wstring().c_str();
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER; // Added OFN_EXPLORER for modern appearance
		if (filter) {
			std::wstring wfilter;
			// Convert filter to wide string and replace | with nulls
			int len = MultiByteToWideChar(CP_ACP, 0, filter, -1, NULL, 0);
			if (len > 0) {
				wfilter.resize(len);
				MultiByteToWideChar(CP_ACP, 0, filter, -1, &wfilter[0], len);
				// Replace | with \0
				for (auto& ch : wfilter) {
					if (ch == L'|') ch = L'\0';
				}
				// Ensure double null termination
				if (!wfilter.empty() && wfilter.back() != L'\0') {
					wfilter.push_back(L'\0');
				}
				ofn.lpstrFilter = wfilter.c_str();
			}
		}
		if (GetOpenFileNameW(&ofn) == TRUE) {
			return std::filesystem::path(ofn.lpstrFile);
		}
		return std::filesystem::path();
	}
}
#endif