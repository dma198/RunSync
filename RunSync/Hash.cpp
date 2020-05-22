#include <iostream> 
#include <sstream>
#include <filesystem> 

#include "SystemUtils.h"
#include "Hash.h"

using namespace RunSync;

#pragma region Constants
constexpr auto BUFSIZE = 1024;
constexpr WCHAR rgbDigits[] = L"0123456789ABCDEF";
static const BYTE rgbIV[] =
{
    0x20, 0x11, 0x05, 0x27, 0x20, 0x17, 0x03, 0x03,
    0x19, 0x70, 0x06, 0x29, 0x0D, 0x0F, 0x0A, 0x01
};

#pragma endregion

#pragma region Hash Calculation

wstring Hash::getDirectoryHash(wstring path)
{
    wstring res(L"");
    auto ses = HashOpen();
    if (ses.hHash != 0)
    {
        try
        {
            vector<wstring> files;
            for (const auto& entry : filesystem::recursive_directory_iterator(path))
            {
                if (filesystem::is_regular_file(entry.path()))
                {
                    files.push_back(entry.path().wstring());
                }
            }

            //Must sort because files order is not guaranteed and in result hash
            //in 2 directories with same content might be different
            std::sort(files.begin(),files.end()); 

            for (auto f : files)
            {
                CalcFileHash(ses,f);
            }
            
            res = getHash(ses);
        }
        catch (...) {}
        HashClose(ses);
    }
    return res;
}

wstring Hash::getFileHash(wstring path)
{
    wstring res(L"");
    
    auto ses = HashOpen();
    if (ses.hHash != 0)
    {
        try
        {
            int cnt = 0;
            if (filesystem::is_regular_file(path))
            {
                CalcFileHash(ses,path);
                cnt++;
            }

            res = getHash(ses);
        }
        catch (...){}
        HashClose(ses);
    }
    return res;
}

EncryptionSession Hash::HashOpen()
{
    EncryptionSession res;
    res.hKey = 0;
    res.hAlg = 0;
    res.hHash = 0;
    res.hashSize = 0;
    res.keyObjSize = 0;
    res.keyObj = nullptr;
    res.pbIV = nullptr;
    if (BCryptOpenAlgorithmProvider(&res.hAlg, BCRYPT_SHA1_ALGORITHM, NULL, 0) == 0)
    {
        DWORD sz;
        if (BCryptGetProperty(res.hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE) & (res.keyObjSize), sizeof(DWORD), &sz, 0) != 0)
            return res;
        res.keyObj = (PBYTE)HeapAlloc(GetProcessHeap(), 0, res.keyObjSize);
        if (BCryptGetProperty(res.hAlg, BCRYPT_HASH_LENGTH, (PBYTE) & (res.hashSize), sizeof(DWORD), &sz, 0) != 0) 
        {
            HeapFree(GetProcessHeap(), 0, res.keyObj); res.keyObj = nullptr;
            return res;
        }        
        if (BCryptCreateHash(res.hAlg, &res.hHash, res.keyObj, res.keyObjSize, NULL, 0, 0) != 0)
        {
            HeapFree(GetProcessHeap(), 0, res.keyObj); res.keyObj = nullptr;            
            res.hHash = 0;
        }
    }
    return res;
}

void Hash::HashClose(EncryptionSession& ses)
{
    BCryptCloseAlgorithmProvider(ses.hAlg, 0);
    if(ses.hHash)BCryptDestroyHash(ses.hHash);
    if(ses.keyObj)HeapFree(GetProcessHeap(), 0, ses.keyObj);
}

void Hash::CalcFileHash(EncryptionSession& ses, const wstring fname)
{
    BYTE rgbFile[BUFSIZE];

    auto hFile = CreateFile(fname.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    DWORD cbRead = 0;

    if (hFile != INVALID_HANDLE_VALUE)
    {
        try
        {
            while (ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL))
            {
                if (cbRead == 0)
                {
                    break;
                }

                BCryptHashData(ses.hHash, rgbFile, cbRead, 0);
            }
        }
        catch (...) {}
        CloseHandle(hFile);

    }


}

wstring Hash::getHash(EncryptionSession& ses)
{
    wstringstream res(L"");
    auto pbHash = (PBYTE)HeapAlloc(GetProcessHeap(), 0, ses.hashSize);

    if (BCryptFinishHash(ses.hHash, pbHash, ses.hashSize, 0) == 0)
    {
        for (DWORD i = 0; i < ses.hashSize; i++)
        {
            res << rgbDigits[pbHash[i] >> 4] << rgbDigits[pbHash[i] & 0xf];
        }
    }

    HeapFree(GetProcessHeap(), 0, pbHash);

    return res.str();

}

#pragma endregion

#pragma region Encryption/Decryption

EncryptionSession EncryptionSessionOpen();
void EncryptionSessionClose(EncryptionSession& ses);

wstring Hash::Encrypt(wstring txt)
{

    wstring res = L"";
    auto ses = EncryptionSessionOpen();
    if (ses.keyObjSize > 0)
    {
        DWORD sz;
        if (BCryptEncrypt(ses.hKey,
            (PBYTE)(txt.c_str()),
            (ULONG)((txt.size() + 1) * sizeof(WCHAR)),
            NULL,
            ses.pbIV,
            ses.blockSize,
            NULL,
            0,
            &sz,
            BCRYPT_BLOCK_PADDING) == 0)
        {
            PBYTE enTxt = (PBYTE)HeapAlloc(GetProcessHeap(), 0, sz); 
            if (BCryptEncrypt(ses.hKey,
                (PBYTE)(txt.c_str()),
                (ULONG)((txt.size() + 1) * sizeof(WCHAR)),
                NULL,
                ses.pbIV,
                ses.blockSize,
                enTxt,
                sz,
                &sz,
                BCRYPT_BLOCK_PADDING) == 0)
            {
                res = SystemUtils::BinToHex(enTxt, sz);
            }
            HeapFree(GetProcessHeap(), 0, enTxt);
        }

        EncryptionSessionClose(ses);
    }


    return res;
}

wstring Hash::Decrypt(wstring txt)
{
    wstring res = L"";
    auto ses = EncryptionSessionOpen();
    if (ses.keyObjSize > 0)
    {
        DWORD sz;
        size_t isz;
        auto ibuf = SystemUtils::HexToBin(txt, &isz);
        if (BCryptDecrypt(ses.hKey,
            (PBYTE)ibuf,
            (ULONG)isz,
            NULL,
            ses.pbIV,
            ses.blockSize,
            NULL,
            0,
            &sz,
            BCRYPT_BLOCK_PADDING) == 0)
        {
            PBYTE decTxt = (PBYTE)HeapAlloc(GetProcessHeap(), 0, sz);
            if (BCryptDecrypt(ses.hKey,
                (PBYTE)ibuf,
                (ULONG)isz,
                NULL,
                ses.pbIV,
                ses.blockSize,
                decTxt,
                sz,
                &sz,
                BCRYPT_BLOCK_PADDING) == 0)
            {
                res = res = wstring((PWSTR)decTxt);
            }
            HeapFree(GetProcessHeap(), 0, decTxt);
        }

        delete[] ibuf;

        EncryptionSessionClose(ses);
    }

    return res;
}

string GetSecret()
{
    CHAR buf[MAX_PATH];
    DWORD len;
    GetComputerNameA(buf, &len);
    string res(buf);
    reverse(res.begin(), res.end());
    if (res.size() > 15) res = res.substr(0, 15);
    while (res.size() < 15) res += "$";
    return res;
}

EncryptionSession EncryptionSessionOpen()
{
    EncryptionSession res;
    res.hKey = 0;
    res.hAlg = 0;
    res.keyObj = nullptr;
    res.pbIV = nullptr;
    res.hHash = 0;
    if (BCryptOpenAlgorithmProvider(&res.hAlg, BCRYPT_AES_ALGORITHM, NULL, 0) == 0)
    {
        DWORD sz;
        if (BCryptGetProperty(res.hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&(res.keyObjSize), sizeof(DWORD), &sz, 0) != 0)
            return res;
       
        res.keyObj = (PBYTE)HeapAlloc(GetProcessHeap(), 0, res.keyObjSize);

        if (BCryptGetProperty(res.hAlg, BCRYPT_BLOCK_LENGTH, (PBYTE)&(res.blockSize), sizeof(DWORD), &sz, 0) != 0)
        {
            HeapFree(GetProcessHeap(), 0, res.keyObj); res.keyObj = nullptr; res.keyObjSize = 0;
            res.keyObjSize = 0;
            res.blockSize = 0;
            return res;
        }

        auto secret = GetSecret();

        if (BCryptGenerateSymmetricKey(res.hAlg, &res.hKey, res.keyObj, (ULONG)res.keyObjSize, (PBYTE)secret.c_str(), (ULONG)secret.size() + 1, 0) == 0)
        {
            res.pbIV = (PBYTE)HeapAlloc(GetProcessHeap(), 0, res.blockSize);
            memcpy(res.pbIV, rgbIV, res.blockSize);
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, res.keyObj); res.keyObj = nullptr; res.keyObjSize = 0;
            res.keyObjSize = 0;
        }
    }
    return res;
}

void EncryptionSessionClose(EncryptionSession& ses)
{
    if (ses.keyObjSize != 0)BCryptDestroyKey(ses.hKey);
    if(ses.hAlg != 0) BCryptCloseAlgorithmProvider(ses.hAlg, 0);
    if(ses.keyObjSize != 0)HeapFree(GetProcessHeap(), 0, ses.keyObj); 
    if (ses.blockSize != 0)HeapFree(GetProcessHeap(), 0, ses.pbIV);
}

#pragma endregion