#include <future>
#include <chrono>
#include <mutex>
#include <concurrent_queue.h>

#include "AppConfig.h"
#include "SystemUtils.h"
#include "OptionsDialog.h"
#include "MainWindow.h"
#include "Hash.h"
#include "AppManager.h"


using namespace RunSync;
using namespace concurrency;


#pragma region Constants
const int STATUS_REFRESH_INTERVAL_MS = 3000;
const int VERSION_CHECK_INTERVAL_MS = 10000;
const wstring VERSION_FILE = L"version";

constexpr wchar_t FS = filesystem::path::preferred_separator;
#pragma endregion

#pragma region Global Variables

list<AppInfo> AppManager::apps;
shared_ptr<list<AppInfo>> AppManager::appsShared(&(AppManager::apps));

mutex mtxCurrentOperation;
Operations currentOperation = Operations::opNone;
wstring currentOperationApp = L"";

thread *trStatus;

bool stop = false;
shared_ptr<bool> stopShared(&stop);
chrono::steady_clock::time_point lastVersionCheck = chrono::steady_clock::now();


#pragma endregion


pair<wstring, Operations> AppManager::GetCurrentAppOperation()
{
	lock_guard lock(mtxCurrentOperation);
	return make_pair(currentOperationApp,currentOperation);
}


AppRunStatuses AppManager::GetAppRunningStatus(AppInfo& ap)
{
	if (ap.RunStatus == AppRunStatuses::rstNotInstalled ||
		ap.RunStatus == AppRunStatuses::rstInstalling ||
		ap.RunStatus == AppRunStatuses::rstUpgrading
		) return ap.RunStatus;

	AppRunStatuses res = AppRunStatuses::rstUndefined;
	auto pid = SystemUtils::GetPID(ap.FullPath);
	if (pid <= 0)
	{
		res = AppRunStatuses::rstStopped;
	}
	else {
		auto hWnd = SystemUtils::GetMainWindow(pid);
		WINDOWPLACEMENT wp;
		GetWindowPlacement(hWnd, &wp);
		if (wp.showCmd == SW_SHOWMAXIMIZED)
			res = AppRunStatuses::rstRunMaximized;
		else if (wp.showCmd == SW_SHOWMINIMIZED)
			res = AppRunStatuses::rstRunMinimized;
		else
			res = AppRunStatuses::rstRunOpened;
	}
	return res;
}

void AppManager::GetStatus()
{
	while (!*stopShared)
	{
		bool changed = false;
		for (auto ap=(*appsShared).begin();ap != (*appsShared).end();ap++)
		{			
			auto prevStatus = ap->RunStatus;

			ap->RunStatus = GetAppRunningStatus(*ap);

			changed = (prevStatus!=ap->RunStatus);
		}

		auto now = chrono::steady_clock::now();
		if (chrono::duration_cast<chrono::milliseconds>(now - lastVersionCheck).count() >= VERSION_CHECK_INTERVAL_MS)
		{
			for (auto ap = (*appsShared).begin(); ap != (*appsShared).end(); ap++)
			{
				ManageVersion(*ap);
			}
			lastVersionCheck = chrono::steady_clock::now();
		}


		if (changed)
		{
			OptionsDialog::Update();
		}

		Sleep(STATUS_REFRESH_INTERVAL_MS);
	}
}

void AppManager::Init()
{
	trStatus = new thread(GetStatus);
}

bool AppManager::ManageVersion(AppInfo& app)
{
	{
		lock_guard lock(mtxCurrentOperation);
		if (currentOperation != Operations::opNone) return false;
		currentOperation = Operations::opCheckForUpdates;
	}

	SystemUtils::LoginToNetworkShared(AppConfig::getDst(), AppConfig::getSrcUser(), AppConfig::getSrcPassword());

	auto lDir = filesystem::path(app.FullPath).parent_path();
	auto lVerFile = lDir.append(VERSION_FILE);
	auto rDir = filesystem::path(AppConfig::getSrc()).append(app.AppName);
	auto rVerFile = rDir.append(VERSION_FILE);
	if (!filesystem::exists(rDir)) // Source directory not found
	{
		app.VersionStatus = AppVersionStatuses::vstUndefined;
		OptionsDialog::setErrorMsg(L"Source folder not found or not accessable!");
		return false; // No version management
	}

	if (!filesystem::exists(rVerFile)) // Version directory not found
	{
		app.VersionStatus = AppVersionStatuses::vstUndefined;
		OptionsDialog::setErrorMsg(L"Version file not found in source folder!");
		return false; // No version management
	}

	OptionsDialog::setErrorMsg(L"");
	try
	{
		auto hshLocal = Hash::getFileHash(lVerFile);
		auto hshRemote = Hash::getFileHash(rVerFile);
		if (hshRemote == L"")
		{
			app.VersionStatus = AppVersionStatuses::vstUndefined;
			OptionsDialog::Update();
			return false; // No version management
		}

		if (hshLocal != hshRemote)
		{
			app.VersionStatus = AppVersionStatuses::vstObsolete;
			OptionsDialog::Update();
			UpgradeAppAsync(app);
			return true;
		}

		app.VersionStatus = AppVersionStatuses::vstUpToDate;
		OptionsDialog::Update();
	}
	catch (...)
	{
		app.VersionStatus = AppVersionStatuses::vstUndefined;
		OptionsDialog::setErrorMsg(L"Error happens when tried to check for new version");
	}
	// Free operation
	{
		lock_guard lock(mtxCurrentOperation);
		if(currentOperation == Operations::opCheckForUpdates) 
			currentOperation = Operations::opNone;
	}

	return false;
}

AppInfo& AppManager::addApp(wstring appName)
{
	for (auto it = appsShared->begin();it != appsShared->end(); it++)
	{
		if (it->AppName == appName)
		{
			return *it;
		}
	}
	
	AppInfo& res(appsShared->emplace_back(AppInfo(appName, AppGetFullDstExePath(appName))));

	OptionsDialog::Update();

	return res;
}

void AppManager::removeApp(wstring appName)
{
	for (auto it = appsShared->begin(); it != appsShared->end(); it++)
		if (it->AppName == appName)
		{
			appsShared->erase(it);
			break;
		}

	OptionsDialog::Update();
}

void AppManager::UpgradeAppAsync(AppInfo& app)
{
	{
		lock_guard lock(mtxCurrentOperation);
		if (currentOperation != Operations::opCheckForUpdates) return;
		currentOperationApp = app.AppName;
		currentOperation = Operations::opUpgrade;
		app.RunStatus = AppRunStatuses::rstUpgrading;
		OptionsDialog::Update();
	}


	thread([&app] {
		{
			lock_guard lock(mtxCurrentOperation);
			if (currentOperation != Operations::opUpgrade) return;
		}

		UINT ShowCmd = SW_SHOW;
		auto pid = SystemUtils::GetPID(app.FullPath);
		if (pid > 0)
		{
			auto hwnd = SystemUtils::GetMainWindow(pid);
			if (hwnd > 0)
			{
				if (hwnd == GetForegroundWindow())
				{
					{
						lock_guard lock(mtxCurrentOperation);
						currentOperation = Operations::opNone;
					}
					return;
				}

				WINDOWPLACEMENT wp;
				if(GetWindowPlacement(hwnd, &wp))
				 ShowCmd = wp.showCmd; //Remember running window state to restore after restarting

				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, pid);
				if (hProcess != NULL)
				{
					TerminateProcess(hProcess, 9);
					CloseHandle(hProcess);
				}
			}
		}

		SystemUtils::LoginToNetworkShared(AppConfig::getDst(),AppConfig::getSrcUser(),AppConfig::getSrcPassword());

		wstring dst(AppConfig::getDst() + FS + app.AppName);
		wstring src(AppConfig::getSrc() + FS + app.AppName);
		wstring dstTmp = dst + L".$tmp$";

		try
		{
			auto srcHash = Hash::getDirectoryHash(src);
			if (filesystem::exists(dstTmp)) filesystem::remove_all(dstTmp);
			if (!filesystem::exists(AppConfig::getDst()))
				filesystem::create_directory(AppConfig::getDst());
			filesystem::copy(src.c_str(), dstTmp.c_str(), filesystem::copy_options::recursive);
			auto dstHash = Hash::getDirectoryHash(dstTmp);
			if (dstHash == srcHash)
			{
				if (filesystem::exists(dst)) filesystem::remove_all(dst);
				filesystem::rename(dstTmp, dst);
				app.VersionStatus = AppVersionStatuses::vstUpToDate;
			}
			else
				filesystem::remove_all(dstTmp);

			app.RunStatus = AppRunStatuses::rstStopped;
			OptionsDialog::Update();

		}
		catch (...) {}

		{
			lock_guard lock(mtxCurrentOperation);
			currentOperation = Operations::opNone;
		}

		if (pid > 0)
		{
			AppStart(app.AppName, ShowCmd);
		}

		}).detach();
}

void AppManager::InstallAppAsync(AppInfo& app)
{
	thread([&app] {
		// Wait finishing of current operation
		while (true) 
		{
			{
				lock_guard lock(mtxCurrentOperation);
				if (currentOperation == Operations::opNone)
				{					
					if (app.RunStatus != AppRunStatuses::rstNotInstalled)return; //Application is already installing or installed
					currentOperationApp = app.AppName;
					currentOperation = Operations::opInstall;
					app.RunStatus = AppRunStatuses::rstInstalling;
					OptionsDialog::Update();
					break;
				}
			}
			Sleep(10);
		}

		SystemUtils::LoginToNetworkShared(AppConfig::getDst(), AppConfig::getSrcUser(), AppConfig::getSrcPassword());

		wstring dst(AppConfig::getDst() + FS + app.AppName);
		wstring src(AppConfig::getSrc() + FS + app.AppName);
		wstring dstTmp = dst + L".$tmp$";

		auto toBeRemoved = false;
		auto success = false;

		if (filesystem::exists(src))
		{
			try
			{
				auto srcHash = Hash::getDirectoryHash(src);
				if (filesystem::exists(dst)) filesystem::remove_all(dst);
				if (filesystem::exists(dstTmp)) filesystem::remove_all(dstTmp);
				if (!filesystem::exists(AppConfig::getDst()))
					filesystem::create_directory(AppConfig::getDst());
				filesystem::copy(src.c_str(), dstTmp.c_str(), filesystem::copy_options::recursive);
				auto dstHash = Hash::getDirectoryHash(dstTmp);
				
				if (dstHash == srcHash)
				{
					filesystem::rename(dstTmp, dst);

					// Create desktop shortcut				
					wstring sht = SystemUtils::GetDesktopPath() + FS + app.AppName + L".lnk";
					if (!filesystem::exists(sht))
					{
						wstring cmd = SystemUtils::GetRunningInstanceFileName();
						SystemUtils::CreateLink(cmd, app.AppName, sht, app.AppName);
					}

					{
						lock_guard lock(mtxCurrentOperation);
						app.RunStatus = AppRunStatuses::rstStopped;
						app.VersionStatus = AppVersionStatuses::vstUpToDate;
						currentOperation = Operations::opNone;
					}
					success = true;
					AppStart(app.AppName);
				}
				else
				{
					filesystem::remove_all(dstTmp);
				}
			}
			catch (...) {}
		}
		else 
		{
			toBeRemoved = true;			
		}
		// Reset current operation in case 
		if(!success)
		{
			lock_guard lock(mtxCurrentOperation);
			app.RunStatus = AppRunStatuses::rstNotInstalled;
			currentOperation = Operations::opNone;
		}

		if (toBeRemoved)
		{
			wstring msg = L"Application " + app.AppName + L" is not found in " + dst +
				L" and can not be installed from " + src + L" (not found/ can't access to there)";
			MessageBox(MainWindow::getHandle(), msg.c_str(), L"Error", MB_ICONERROR);
			removeApp(app.AppName);
		}

		return;
		}).detach();
}

wstring AppManager::AppGetFullDstExePath(const wstring& appName)
{
	return AppConfig::getDst() + FS + appName + FS + appName + L".exe";
};

void AppManager::AppStart(const wstring& appName, UINT showCmd)
{
	AppInfo& app(addApp(appName));

	if (!filesystem::exists(app.FullPath)) {
		app.RunStatus = AppRunStatuses::rstNotInstalled;
		OptionsDialog::Update();
		InstallAppAsync(app);
		OptionsDialog::Update();
		return;
	}

	if(ManageVersion(app)) return;

	auto st = GetAppRunningStatus(app);

	if (st == AppRunStatuses::rstStopped)
	{
		STARTUPINFO si;
		if (showCmd > 0)si.wShowWindow = showCmd;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		wstring startupDir = filesystem::path(app.FullPath).parent_path();

		if (CreateProcess(
			NULL,
			(LPWSTR)app.FullPath.c_str(),
			NULL, NULL, FALSE, 0, NULL, startupDir.c_str(), &si, &pi))
		{
		}

		if (showCmd > 0)
		{
			Sleep(15000);
			auto hwnd = SystemUtils::GetMainWindow(pi.dwProcessId);
			if (hwnd > 0)ShowWindow(hwnd,showCmd);
		}
	
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	if (st == AppRunStatuses::rstRunMinimized)
	{
		auto pid = SystemUtils::GetPID(app.FullPath);
		auto hwnd = SystemUtils::GetMainWindow(pid);
		if (hwnd > 0)
		{
			PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
		}
	}

}
