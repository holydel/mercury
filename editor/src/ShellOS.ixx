module;
#include <mercury_api.h>
#include <string>
#include <filesystem>
#include <functional>
#include <thread>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <vector>

#ifdef MERCURY_LL_OS_WIN32
#include <windows.h>
#include <shlobj.h>
#include <cwctype>
#endif

export module ShellOS;

#ifdef MERCURY_LL_OS_WIN32

export namespace ShellOS {
	export enum WatcherAction
	{
		FileNameChanged = FILE_NOTIFY_CHANGE_FILE_NAME,
		DirNameChanged = FILE_NOTIFY_CHANGE_DIR_NAME,
		AttributeChanged = FILE_NOTIFY_CHANGE_ATTRIBUTES,
		LastWriteChanged = FILE_NOTIFY_CHANGE_LAST_WRITE,
		SizeChanged = FILE_NOTIFY_CHANGE_SIZE
	};

	export using FolderWatcherFn = void(const std::filesystem::path&, WatcherAction);
	export using FolderWatcherCallback = std::function<FolderWatcherFn>;
}

std::unordered_map<std::filesystem::path, ShellOS::FolderWatcherCallback> g_folderWatchers;

struct FolderWatchContext
{
	std::filesystem::path root;
	ShellOS::FolderWatcherCallback callback;
	std::vector<std::wstring> excludedTopLevel; // lowercase, no trailing slashes
	HANDLE dirHandle = INVALID_HANDLE_VALUE;
	std::atomic<bool> running{ false };
	std::thread worker;

	~FolderWatchContext() { Stop(); }

	static std::wstring ToLower(const std::wstring& s)
	{
		std::wstring out;
		out.reserve(s.size());
		for (wchar_t c : s) out.push_back(std::towlower(c));
		return out;
	}

	// Returns true if the relative (as delivered by ReadDirectoryChangesW) is excluded
	bool IsExcluded(const std::wstring_view& rel) const
	{
		if (excludedTopLevel.empty()) return false;

		// Find first path separator (if any)
		size_t firstSep = std::wstring::npos;
		for (size_t i = 0; i < rel.size(); ++i)
		{
			wchar_t c = rel[i];
			if (c == L'\\' || c == L'/')
			{
				firstSep = i;
				break;
			}
		}

		std::wstring_view firstComponent = (firstSep == std::wstring::npos) ? rel : rel.substr(0, firstSep);

		// Lowercase compare on the fly (avoid allocating if not matching first char)
		for (const auto& ex : excludedTopLevel)
		{
			if (!ex.empty())
			{
				if (std::towlower(firstComponent[0]) != ex[0]) continue;
				if (firstComponent.size() == ex.size())
				{
					bool equal = _wcsnicmp(firstComponent.data(), ex.c_str(), (unsigned)ex.size()) == 0;
					if (equal) return true;
				}
			}
		}
		return false;
	}

	void Start()
	{
		if (running.load()) return;

		std::wstring wroot = root.wstring();
		dirHandle = ::CreateFileW(
			wroot.c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS,
			nullptr);

		if (dirHandle == INVALID_HANDLE_VALUE)
			return;

		running = true;

		worker = std::thread([self = this]()
		{
			const DWORD notifyFilter =
				FILE_NOTIFY_CHANGE_FILE_NAME |
				FILE_NOTIFY_CHANGE_DIR_NAME |
				FILE_NOTIFY_CHANGE_LAST_WRITE |
				FILE_NOTIFY_CHANGE_SIZE;

			alignas(FILE_NOTIFY_INFORMATION) BYTE buffer[16 * 1024];

			while (self->running.load())
			{
				DWORD bytesReturned = 0;
				BOOL ok = ::ReadDirectoryChangesW(
					self->dirHandle,
					buffer,
					DWORD(sizeof(buffer)),
					TRUE, // recursive
					notifyFilter,
					&bytesReturned,
					nullptr,
					nullptr);

				if (!ok)
				{
					DWORD err = ::GetLastError();
					if (err == ERROR_ACCESS_DENIED || err == ERROR_INVALID_HANDLE)
						break;
					::Sleep(30);
					continue;
				}

				if (bytesReturned == 0)
					continue;

				auto* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);

				for (;;)
				{
					std::wstring name(fni->FileName, fni->FileNameLength / sizeof(WCHAR));

					// Quick exclusion check on relative name
					if (!self->excludedTopLevel.empty())
					{
						if (self->IsExcluded(name))
						{
							if (fni->NextEntryOffset == 0) break;
							fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
								reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
							continue;
						}
					}

					std::filesystem::path changed = self->root / name;

					try { if (self->callback) self->callback(changed, (ShellOS::WatcherAction)fni->Action); }
					catch (...) {}

					if (fni->NextEntryOffset == 0)
						break;

					fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
						reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
				}
			}
		});
	}

	void Stop()
	{
		if (!running.load()) return;
		running = false;

		if (dirHandle != INVALID_HANDLE_VALUE)
		{
			::CancelIoEx(dirHandle, nullptr);
			::CloseHandle(dirHandle);
			dirHandle = INVALID_HANDLE_VALUE;
		}
		if (worker.joinable())
			worker.join();
	}
};

// Path -> active watcher
static std::unordered_map<std::filesystem::path, std::shared_ptr<FolderWatchContext>> g_activeWatchers;

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

	export std::filesystem::path SelectFolderDialog(const std::filesystem::path& initialPath = "") {
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

	export std::filesystem::path SelectFileDialog(const char* filter = "", const std::filesystem::path& initialPath = "") {
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

	// Extended: exclusions are top-level folder names to ignore (e.g. {".vs","build","intermediate"})
	export void SetupFolderWatcher(const std::filesystem::path& path,
		ShellOS::FolderWatcherCallback callback,
		const std::vector<std::string>& excludedTopLevelDirs = {})
	{
		if (path.empty() || !std::filesystem::exists(path) || !std::filesystem::is_directory(path))
			return;

		// If already watching, stop previous
		if (auto it = g_activeWatchers.find(path); it != g_activeWatchers.end())
		{
			it->second->Stop();
			g_activeWatchers.erase(it);
		}

		g_folderWatchers[path] = callback;

		auto watcher = std::make_shared<FolderWatchContext>();
		watcher->root = std::filesystem::canonical(path);
		watcher->callback = std::move(callback);

		watcher->excludedTopLevel.reserve(excludedTopLevelDirs.size());
		for (auto& s : excludedTopLevelDirs)
		{
			if (s.empty()) continue;
			std::wstring ws(s.begin(), s.end());
			watcher->excludedTopLevel.push_back(FolderWatchContext::ToLower(ws));
		}

		watcher->Start();

		if (watcher->dirHandle == INVALID_HANDLE_VALUE)
			return;

		g_activeWatchers[watcher->root] = watcher;
	}

	export void RemoveFolderWatcher(const std::filesystem::path& path)
	{
		if (auto it = g_activeWatchers.find(path); it != g_activeWatchers.end())
		{
			it->second->Stop();
			g_activeWatchers.erase(it);
		}
		g_folderWatchers.erase(path);
	}

	export void ShutdownAllWatchers()
	{
		for (auto& [p, ctx] : g_activeWatchers)
			ctx->Stop();
		g_activeWatchers.clear();
		g_folderWatchers.clear();
	}
}

#endif