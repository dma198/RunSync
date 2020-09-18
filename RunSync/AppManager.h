#pragma once
#include "framework.h"
#include <list>
#include <tuple>
#include <string>
#include <future>
#include <filesystem>
#include <chrono>

using namespace std;
using namespace chrono;

namespace RunSync
{

	enum class Operations { opUndefined, opNone, opInstall, opCheckForUpdates, opUpgrade };
	enum class AppRunStatuses { rstUndefined, rstNotInstalled, rstInstalling, rstUpgrading, rstRunOpened, rstRunMaximized, rstRunMinimized, rstStopped };
	enum class AppVersionStatuses { vstUndefined, vstUpToDate, vstObsolete, vstUpgradeError };

	struct AppInfo
	{
		AppInfo() 
		{ 
			AppName = FullPath = L""; 
			RunStatus = AppRunStatuses::rstUndefined; 
			VersionStatus = AppVersionStatuses::vstUndefined; 
			LastVersionCheckTime = chrono::steady_clock::now();
			LastUpgradeTime = LastVersionCheckTime;
		};
		AppInfo(wstring appName, wstring fullPath):AppInfo() { AppName = appName; FullPath = fullPath; };
		wstring AppName;
		wstring FullPath;
		AppRunStatuses RunStatus;
		AppVersionStatuses VersionStatus;	
		steady_clock::time_point LastVersionCheckTime;
		steady_clock::time_point LastUpgradeTime;
	};

	class AppManager
	{
	public:
		static void Init();
		static AppInfo& addApp(wstring appName);
		static void removeApp(wstring appName);
		static void AppStart(const wstring& appName, UINT showCmd = 0);
		static list<AppInfo>& getApps() { return apps; }
		static pair<wstring,Operations> GetCurrentAppOperation();
	private:
		static list<AppInfo> apps;
		static shared_ptr<list<AppInfo>> appsShared;
		static void InstallAppAsync(AppInfo& app);
		static void UpgradeAppAsync(AppInfo& ap);
		static wstring AppGetFullDstExePath(const wstring& appName);
		static AppRunStatuses GetAppRunningStatus(AppInfo& ap);
		static void GetStatus();
		static bool ManageVersion(AppInfo& res);
	};

}

