#include "stdafx.h"
#include "JsonParse.h"


template<typename T>
CJsonParse<T>::CJsonParse()
{
}

template<typename T>
CJsonParse<T>::~CJsonParse()
{
}

template<typename T>
BOOL CJsonParse<T>::Load(LPCWSTR lpstrFilePath)
{
    BOOL bRet = FALSE;
    m_strFile = lpstrFilePath;

    do
    {
        CAutoVectorPtr<char> pData;
        if (!LoadFile(lpstrFilePath, pData))
            break;

        if (!pData.m_p)
            break;

        m_pJsonData.Allocate(_msize(pData.m_p) / sizeof(char));
        m_pJsonData.Attach(CA2W(pData.Detach(), CP_UTF8));

        if (!m_pJsonData.m_p)
            break;
        bRet = LoadFromData(m_pJsonData.m_p);
    } while (FALSE);
    return 0;
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
BOOL CJsonParse<T>::LoadFile(LPCWSTR lpstrFilePath, CAutoVectorPtr<char>& pFileData)
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
        ULONG nSize = 0;
        {
            ULONGLONG nFileSize = 0;
            file.GetSize(nFileSize);
            if (nFileSize == 0)
            {
                break;
            }
            nSize = (ULONG)nFileSize;
        }

        if (!pFileData.Allocate(nSize + 1))
        {
            break;
        }

        if (FAILED(file.Read(
            pFileData.m_p,
            nSize)))
        {
            break;
        }
        pFileData.m_p[nSize] = '\0';
        file.Close();

        bRet = TRUE;

    } while (FALSE);

    return bRet;
}
