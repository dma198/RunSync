#pragma once

#include "framework.h" 
#include <string>

using namespace std;

namespace RunSync
{
	class BootstrapManager
	{
	public: 
		static bool SendCmdToRunningInstance(const wstring& cmd);
	};
}

