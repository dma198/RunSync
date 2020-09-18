#pragma once

#include "framework.h" 
#include <string>

using namespace std;

namespace RunSync
{

	class SystemUtils
	{
	public:
		static DWORD GetPID(wstring exeFileFullName, bool excludeCurrent=false);
		static HWND GetMainWindow(unsigned long process_id, bool only_visible = true);
		static DWORD GetRunningInstancePID(); // Get PID of other process starts from same exe-file as current (if is)
		static wstring GetRunningInstanceFileName();
		static wstring GetDesktopPath();
		static bool CreateLink(wstring exePath, wstring args, wstring lnkPath, wstring linkDesription);
		static string ToUTF8(const wstring& wstr);
		static wstring FromUTF8(const std::string& str);
		static bool LoginToNetworkShared(wstring path, wstring user, wstring encryptedPassword);
		static wstring BinToHex(PBYTE bytes, size_t size);
		static PBYTE HexToBin(wstring hex, size_t* size); // Required delete[] to result
		static bool EndsWith(wstring const& value, wstring const& ending);		
	private:
		static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam);
		static BOOL IsMainWindow(HWND handle, bool only_visibl);
	};

}

