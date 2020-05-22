#pragma once
#include <string>
#include <windows.h>

using namespace std;

namespace RunSync
{
	class AppConfig
	{
	public:
		static void   Init(const wstring& args);
		static wstring getAppName();		// Get applications name
		static void setAppName(wstring name) { appName = name; };	// Set applications name
		static wstring getSrc();			// Get applications source folder
		static void   setSrc(wstring src);  // Set applications source folder
		static wstring getDst();			// Get applications destination folder
		static void   setDst(wstring dst);  // Set applications destination folder
		static wstring  getSrcUser() { return srcUser; };
		static void  setSrcUser(wstring user) {srcUser=user; };
		static wstring  getSrcPassword() { return srcPassword; };
		static void  setSrcPassword(wstring passwd) { srcPassword = passwd; };
		static RECT   getDesktopRect();
		static void Save();
	
	private:
		static wstring src;
		static wstring dst;
		static wstring appName;
		static wstring srcUser;
		static wstring srcPassword;
		static RECT desktopRect;
		static void LoadFromCmdLine(const wstring& args);
		static void LoadFromJSON();
		static void LoadFromRegistry();
		static bool SaveToRegistry();
		static void SaveToJSON();

	};


}

