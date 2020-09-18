#include <sstream>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <codecvt>

#include "JSON.h"

#include "Hash.h"
#include "SystemUtils.h"
#include "AppConfig.h"

using namespace std;
using namespace RunSync;


auto const sKey = L"SOFTWARE\\Danieli Automation\\RunSync";
wstring const vSrc = L"AppsSrc";
wstring const vDst = L"AppsDst";
wstring const vSrcUser = L"SrcUser";
wstring const vSrcPassword = L"SrcPassword";

wstring AppConfig::src = L"";
wstring AppConfig::dst = L"";
wstring AppConfig::appName = L"";
wstring AppConfig::srcUser = L"";
wstring AppConfig::srcPassword = L"";

RECT AppConfig::desktopRect;

void AppConfig::Init(const wstring& args)
{

	LoadFromCmdLine(args);

    if (src == L"" || dst != L"")
    {
		LoadFromRegistry();		
		if (src == L"" || dst != L"")
			LoadFromJSON();
    }

	auto hDsk = GetDesktopWindow();
	GetWindowRect(hDsk, &desktopRect);
}

wstring AppConfig::getAppName()
{
	return appName;
}

wstring AppConfig::getSrc()
{
	return src;
}

void AppConfig::setSrc(wstring src)
{
	AppConfig::src = src;
}

wstring AppConfig::getDst()
{
	return dst;
}

void AppConfig::setDst(wstring dst)
{
	AppConfig::dst = dst;
}

// Private Methods
void AppConfig::LoadFromCmdLine(const wstring& args)
{	
	wstringstream ss(args);
	wstring token;
	int cnt = 0;
	while (getline(ss, token, L' ')) {
		if (token != L"")
		{
			cnt++;
			if (cnt == 1)
			{
				appName = token;
				continue;
			}
			if (token.find_first_of(L"--") == 0)
			{
				size_t ieq = token.find_first_of(L'=');

				auto var = token.substr(2, ieq - 2);
				transform(var.begin(), var.end(), var.begin(), ::tolower);
				auto val = token.substr(ieq + 1, token.length() - ieq - 1);
				if (var == L"src") src = val;
				else if (var == L"dst") dst = val;
				else if (var == L"user") srcUser = val;
				else if (var == L"password") srcPassword = val;
			}

		}
	}
}

void AppConfig::LoadFromJSON()
{
	auto fname = filesystem::path(SystemUtils::GetRunningInstanceFileName());
	fname.replace_extension(filesystem::path(L".json"));
	if (filesystem::exists(fname))
	{
		
		ifstream fs(fname);
		std::stringstream ss;
		ss << fs.rdbuf();
		fs.close();

		wstring json = SystemUtils::FromUTF8(ss.str());

		JSONValue* value = JSON::Parse(json.c_str());
		if (value != nullptr)
		{
			if (value->IsObject())
			{
				JSONObject root = value->AsObject();
				if (root.find(vSrc) != root.end())
					if (src == L"")src = root[vSrc]->AsString();
				if (root.find(vDst) != root.end())
					if (dst == L"")dst = root[vDst]->AsString();
				if (root.find(vSrcUser) != root.end())
					if (srcUser == L"")srcUser = root[vSrcUser]->AsString();
				if (root.find(vSrcPassword) != root.end())
					if (srcPassword == L"")srcPassword = root[vSrcPassword]->AsString();

			}
			delete value;
		}
		
	}
}

void AppConfig::SaveToJSON()
{
	auto fname = filesystem::path(SystemUtils::GetRunningInstanceFileName());
	fname.replace_extension(filesystem::path(L".json"));

	JSONObject root;
	root[vSrc] = new JSONValue(src);
	root[vDst] = new JSONValue(dst);
	root[vSrcUser] = new JSONValue(srcUser);
	root[vSrcPassword] = new JSONValue(srcPassword);
	JSONValue* value = new JSONValue(root);
	auto s = SystemUtils::ToUTF8(value->Stringify(true));
	delete value;
	ofstream fs(fname);
	fs << s;
	fs.close();
}


void AppConfig::LoadFromRegistry()
{

	//auto enTxt =  Hash::Encrypt(L"Hello");
	//auto decTxt = Hash::Decrypt(enTxt);
	//Hash::getFileHash(L"AAA");

    HKEY hKey;
    if (RegConnectRegistry(nullptr, HKEY_CURRENT_USER, &hKey) == ERROR_SUCCESS)
    {

        HKEY key;
        if (RegOpenKey(hKey, sKey, &key) == ERROR_SUCCESS)
        {
            WCHAR tmp[MAX_PATH];
            DWORD type, len = sizeof(wchar_t) * MAX_PATH;
            if (src == L"")
            {
                if (RegGetValue(hKey, sKey, vSrc.c_str(), RRF_RT_REG_SZ, &type, (PVOID)tmp, &len) == ERROR_SUCCESS)
                    src = tmp;
            }
            if (dst == L"")
            {
                if (RegGetValue(hKey, sKey, vDst.c_str(), RRF_RT_REG_SZ, &type, (PVOID)tmp, &len) == ERROR_SUCCESS)
                    dst = tmp;
            }
			if (srcUser == L"")
			{
				if (RegGetValue(hKey, sKey, srcUser.c_str(), RRF_RT_REG_SZ, &type, (PVOID)tmp, &len) == ERROR_SUCCESS)
					srcUser = tmp;
			}
			if (srcPassword == L"")
			{
				if (RegGetValue(hKey, sKey, srcPassword.c_str(), RRF_RT_REG_SZ, &type, (PVOID)tmp, &len) == ERROR_SUCCESS)
					srcPassword = tmp;
			}
			RegCloseKey(key);
        }
		RegCloseKey(hKey);
    }
}

bool AppConfig::SaveToRegistry()
{
	auto res = true;
    HKEY hKey;
	if (RegConnectRegistry(nullptr, HKEY_CURRENT_USER, &hKey) == ERROR_SUCCESS)
	{
		if(RegSetKeyValue(hKey, sKey, vSrc.c_str(), RRF_RT_REG_SZ, src.c_str(), (DWORD)((src.size()+1) * sizeof(wchar_t))) != ERROR_SUCCESS)
			res=false;
		if(res && RegSetKeyValue(hKey, sKey, vDst.c_str(), RRF_RT_REG_SZ, dst.c_str(), (DWORD)((dst.size()+1) * sizeof(wchar_t))) != ERROR_SUCCESS)
			res = false;
		if (res && RegSetKeyValue(hKey, sKey, vSrcUser.c_str(), RRF_RT_REG_SZ, srcUser.c_str(), (DWORD)((srcUser.size() + 1) * sizeof(wchar_t))) != ERROR_SUCCESS)
			res = false;
		if (res && RegSetKeyValue(hKey, sKey, vSrcPassword.c_str(), RRF_RT_REG_SZ, srcPassword.c_str(), (DWORD)((srcPassword.size() + 1) * sizeof(wchar_t))) != ERROR_SUCCESS)
			res = false;
		RegCloseKey(hKey);
	}
	else res = false;
	return res;
}

void AppConfig::Save()
{
	if (!SaveToRegistry())
		SaveToJSON();
}

RECT AppConfig::getDesktopRect()
{
	return desktopRect;
}


