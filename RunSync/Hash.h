#pragma once
#include <string>
#include "framework.h"


using namespace std;

namespace RunSync
{
	struct EncryptionSession
	{
		BCRYPT_ALG_HANDLE hAlg;
		BCRYPT_KEY_HANDLE hKey;
		BCRYPT_HASH_HANDLE hHash;
		DWORD keyObjSize;
		DWORD blockSize;
		DWORD hashSize;
		PBYTE keyObj;
		PBYTE pbIV;
	};

	class Hash
	{
	public:
		static wstring getDirectoryHash(wstring path);
		static wstring getFileHash(wstring path);
		static wstring Encrypt(wstring txt);
		static wstring Decrypt(wstring txt);
	private:
		static EncryptionSession HashOpen();
		static void HashClose(EncryptionSession& ses);
		static void CalcFileHash(EncryptionSession& ses,const wstring fname);
		static wstring getHash(EncryptionSession& ses);
	};
}

