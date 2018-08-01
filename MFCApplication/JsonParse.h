#pragma once

#include "json\reader.h"
#include "json\value.h"
#include <atlfile.h>
#include <memory>

template<typename T = char>
class CJsonParse
{
public:
    CJsonParse();
    ~CJsonParse();

    BOOL Load(LPCWSTR lpstrFilePath);
    void UnLoad();
    BOOL LoadFromData(const T* data);
    Json::Value& GetRoot();

private:
    BOOL LoadFile(LPCWSTR lpstrFilePath, CAutoVectorPtr<char>& pFileData, ULONG& nSize);

private:
    ATL::CAtlString m_strFile;
    CAutoVectorPtr<T> m_pJsonData;
    Json::Value m_Jsonroot;
};

template<typename T>
CJsonParse<T>::CJsonParse()
{
}

template<typename T>
CJsonParse<T>::~CJsonParse()
{
}


BOOL UTF82A(CAutoVectorPtr<char>& _in_utf_8, CAutoVectorPtr<char>& out_char, ULONG inSize)
{
    BOOL bRet = FALSE;

    do
    {
        //先将UTF-8转换成wchar
        int wsize = MultiByteToWideChar(CP_UTF8, 0, _in_utf_8.m_p, inSize, NULL, 0);
        if (wsize == 0)
            break;
        std::shared_ptr<wchar_t*> pWideChar = std::make_shared<wchar_t*>(new wchar_t[wsize]);
        if (!(*pWideChar))
            break;
        MultiByteToWideChar(CP_UTF8, 0, _in_utf_8, inSize, *pWideChar, wsize);

        //再将wchar转换成char
        int asize = WideCharToMultiByte(CP_ACP, 0, *pWideChar, wsize, NULL, 0, NULL, 0);
        if (asize == 0)
            break;
        if (!out_char.Allocate(asize + 1))
            break;
        WideCharToMultiByte(CP_ACP, 0, *pWideChar, wsize, out_char.m_p, asize, NULL, 0);
        (out_char.m_p)[asize] = '\0';

        bRet = TRUE;
    } while (FALSE);
   
    return bRet;
}


template<typename T>
BOOL CJsonParse<T>::Load(LPCWSTR lpstrFilePath)
{
    BOOL bRet = FALSE;
    m_strFile = lpstrFilePath;

    do
    {
        CAutoVectorPtr<char> pData;
        ULONG nSize;
        if (!LoadFile(lpstrFilePath, pData, nSize))
            break;

        if (!pData.m_p)
            break;

        if (!UTF82A(pData, m_pJsonData, nSize))
            break;

        if (!m_pJsonData.m_p)
            break;

        bRet = LoadFromData(m_pJsonData.m_p);
        //pData.Free();

    } while (FALSE);

    return bRet;
}

template<typename T>
void CJsonParse<T>::UnLoad()
{
    m_pJsonData.Free();
    m_Jsonroot.clear();
}

template<typename T>
Json::Value & CJsonParse<T>::GetRoot()
{
    return m_Jsonroot;
}

template<>
BOOL CJsonParse<char>::LoadFromData(const char * data)
{
    BOOL bRet = FALSE;
    Json::Reader m_JsonReader;

    do
    {
        if (!data)
            break;

        if (!m_JsonReader.parse(data, m_Jsonroot))
            break;

        bRet = TRUE;
    } while (FALSE);

    return bRet;
}

template<typename T>
BOOL CJsonParse<T>::LoadFile(LPCWSTR lpstrFilePath, CAutoVectorPtr<char>& pFileData, ULONG& nSize)
{
    BOOL bRet = FALSE;

    do
    {
        ATL::CAtlFile file;
        HRESULT hr = file.Create(
            lpstrFilePath,
            GENERIC_READ,
            FILE_SHARE_READ,
            OPEN_EXISTING);
        if (FAILED(hr))
        {
            break;
        }
        nSize = 0;
        {
            ULONGLONG nFileSize = 0;
            file.GetSize(nFileSize);
            if (nFileSize == 0)
            {
                break;
            }
            nSize = (ULONG)nFileSize;
        }

        if (!pFileData.Allocate(nSize))
        {
            break;
        }

        if (FAILED(file.Read(
            pFileData.m_p,
            nSize)))
        {
            break;
        }
        //pFileData.m_p[nSize] = '\0';
        file.Close();

        bRet = TRUE;

    } while (FALSE);

    return bRet;
}

