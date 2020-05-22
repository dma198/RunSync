#include <algorithm>
#include <filesystem>
#include <sstream>

#include "Hash.h"
#include "SystemUtils.h"

using namespace RunSync;


struct handle_data {
	unsigned long process_id;
	HWND window_handle;
};

#pragma region Public Methods

DWORD SystemUtils::GetPID(wstring exeFileFullName,bool excludeCurrent)
{

	DWORD res = 0;

	DWORD pidsCnt = 1024;
	DWORD pidsCntR = 0;
	auto pids = new DWORD[pidsCnt];
	ZeroMemory(pids, pidsCnt);
	
	while (true)
	{
		EnumProcesses(pids, sizeof(DWORD) * pidsCnt, &pidsCntR);
		if (pidsCntR == pidsCnt/sizeof(DWORD))
		{
			delete[] pids;
			pidsCnt += 512;
			pids = new DWORD[pidsCnt];
			ZeroMemory(pids, pidsCnt);
			pidsCntR = 0;
			continue;
		}
		break;
	}
	pidsCnt = pidsCntR / sizeof(DWORD);

	DWORD curPid = 0;
	if(excludeCurrent)
		curPid = GetCurrentProcessId();

	int pCnt = 0;
	WCHAR fpath[MAX_PATH];

	for (size_t i = 0; i < pidsCnt; i++)
	{
		if (excludeCurrent && curPid == pids[i] || pids[i]==0)continue;
	
		HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, pids[i]);
		if (h != nullptr)
		{
			pCnt++;

			ZeroMemory(fpath, MAX_PATH * sizeof(WCHAR));
			if(GetModuleFileNameEx(h,NULL, fpath, MAX_PATH) > 0)			
			{
				wstring tmp1(exeFileFullName);
				wstring tmp2(fpath);
				transform(tmp1.begin(), tmp1.end(), tmp1.begin(), ::tolower);
				transform(tmp2.begin(), tmp2.end(), tmp2.begin(), ::tolower);
				if (tmp1 == tmp2)
				{
					res = pids[i];
				}

			}

			CloseHandle(h);
			if (res > 0)break;
		}
	}


	delete[] pids;

	return res;
}

HWND SystemUtils::GetMainWindow(unsigned long process_id)
{
	handle_data data;
	data.process_id = process_id;
	data.window_handle = 0;
	EnumWindows(EnumWindowsCallback, (LPARAM)&data);
	return data.window_handle;
}

DWORD SystemUtils::GetRunningInstancePID()
{
    WCHAR szFilePath[MAX_PATH];
    GetModuleFileName(NULL, szFilePath, MAX_PATH);
    return GetPID(szFilePath,true);
}

wstring SystemUtils::GetRunningInstanceFileName()
{
	WCHAR szFilePath[MAX_PATH];
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	return wstring(szFilePath);
}

wstring SystemUtils::GetDesktopPath()
{
	WCHAR szFilePath[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, szFilePath);
	return wstring(szFilePath);
}

// Create Windows Shortcut 
bool SystemUtils::CreateLink(wstring exePath,wstring args,wstring lnkPath, wstring lnkDesription)
{
	HRESULT hres = E_FAIL;
	IShellLink* psl;

	CoInitialize(NULL);
	try
	{

		hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
		if (SUCCEEDED(hres))
		{
			IPersistFile* ppf;

			psl->SetPath(exePath.c_str());
			psl->SetArguments(args.c_str());
			psl->SetDescription(lnkDesription.c_str());

			hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

			if (SUCCEEDED(hres))
			{
				hres = ppf->Save(lnkPath.c_str(), TRUE);
				ppf->Release();
			}
			psl->Release();
		}
	}
	catch (...) {}
	CoUninitialize();

	return (hres>0);

}

#pragma endregion

BOOL SystemUtils::IsMainWindow(HWND handle)
{
    return GetWindow(handle, GW_OWNER) == (HWND)0;
}

BOOL CALLBACK SystemUtils::EnumWindowsCallback(HWND handle, LPARAM lParam)
{
    handle_data& data = *(handle_data*)lParam;
    unsigned long process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data.process_id != process_id || !IsMainWindow(handle))
        return TRUE;
    data.window_handle = handle;
    return FALSE;
}

string SystemUtils::ToUTF8(const wstring& wstr)
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

// Convert an UTF8 string to a wide Unicode String
wstring SystemUtils::FromUTF8(const std::string& str)
{
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

bool SystemUtils::LoginToNetworkShared(wstring path, wstring user, wstring encryptedPassword)
{
	auto res = true;
	auto password = Hash::Decrypt(encryptedPassword);

	if (PathIsNetworkPath(path.c_str()) && user != L"")
	{

		NETRESOURCE resource;
		resource.dwType = RESOURCETYPE_DISK;
		resource.lpLocalName = 0;
		WCHAR* lPath = new WCHAR[path.size() + 1];
		lstrcpy(lPath, path.c_str());
		resource.lpRemoteName = lPath;
		resource.lpProvider = 0;
		WNetCancelConnection(lPath, TRUE);
		DWORD result = WNetAddConnection2(&resource, password.c_str(), user.c_str(), CONNECT_TEMPORARY);

		delete[] lPath;
		res = result != NO_ERROR;

	}
	return res;
}

wstring SystemUtils::BinToHex(PBYTE bytes, size_t size)
{
	wstringstream str;
	for (int i = 0; i < size; i++)
		str << std::uppercase << std::setfill(L'0') << std::setw(2) << std::hex << (int)bytes[i];
	return str.str();
}

PBYTE SystemUtils::HexToBin(wstring hex, size_t* size)
{
	*size = 0;
	if (hex.size() == 0)return nullptr;
	*size = hex.size() / 2;
	auto res = new BYTE[*size];
	for (size_t i = 0; i < *size; i++)
	{
		short d;
		wistringstream hs(hex.substr((size_t)i * 2, 2));
		hs >> std::hex >> d;
		res[i] = (CHAR)d;
	}
	return res;
}