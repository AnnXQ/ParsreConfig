#include "stdafx.h"
#include "LocalMapParse.h"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "rapidxml_utils.hpp"

#include <memory>

using namespace rapidxml;

CLocalMapParse::CLocalMapParse()
{

}

CLocalMapParse::~CLocalMapParse()
{
    map<int, CMapInfo*>::iterator iter;

    for (iter = LocalMap.begin(); iter != LocalMap.end(); iter++)
        delete(iter->second);
}


BOOL ReadXmlFile(CString FileName, std::shared_ptr<char*>& content)
{
    CFile mFile;

    if (!mFile.Open(FileName, CFile::modeRead | CFile::typeBinary))
    {
        //读取文件失败
        return FALSE;
    }

    UINT FileSize = (UINT)mFile.GetLength();

    shared_ptr<char*> buf = make_shared<char*>(new char[FileSize]);
    memset(*buf, 0, FileSize);
    mFile.Read(*buf, FileSize);
    mFile.Close();

    //先讲UTF-8转换成wchar
    int wsize = MultiByteToWideChar(CP_UTF8, 0, *buf, FileSize, NULL, 0);
    shared_ptr<wchar_t*> pWideChar = make_shared<wchar_t*>(new wchar_t[wsize]);
    MultiByteToWideChar(CP_UTF8, 0, *buf, FileSize, *pWideChar, wsize);

    //再讲wchar转换成char
    int asize = WideCharToMultiByte(CP_ACP, 0, *pWideChar, wsize, NULL, 0, NULL, 0);
    content = make_shared<char *>(new char[asize + 1]);
    std::shared_ptr<char *>::element_type pChar = *content;
    WideCharToMultiByte(CP_ACP, 0, *pWideChar, wsize, pChar, asize, NULL, 0);
    pChar[asize] = '\0';

    return TRUE;
}

//char->int
unsigned int char2int(char *array)
{
    //只能转换到unsigned int大小
    return (unsigned int) strtol(array, NULL, 10);
}

CMapInfo * ParseDoc(xml_node<> *root, int& id)
{
    xml_attribute<> *attribute = NULL;
    CMapInfo *newItem = NULL;
    //用于暂时存储的变量
    unsigned int build_min = 0;
    unsigned int build_max = 0;
    unsigned int appid = 0;
    unsigned int policy_id = 0;
    unsigned int switch_id = 0;
    unsigned int url_id = 0;
    unsigned int md5_id = 0;

    do
    {
        //CfgItem
        attribute = root->first_attribute();
        if (strcmp(attribute->name(), "id") != 0)
            break;
        id = char2int(attribute->value());
        attribute = attribute->next_attribute();
        if (strcmp(attribute->name(), "name") != 0)
            break;
        CString name(attribute->value());
        attribute = attribute->next_attribute();
        if (strcmp(attribute->name(), "guid") != 0)
            break;
        CString guid(attribute->value());

        //QQPCMgrSupport
        xml_node<> *childroot = root->first_node("QQPCMgrSupport");
        if (childroot == NULL)
            break;
        attribute = childroot->first_attribute();
        if (strcmp(attribute->name(), "build_min") == 0)
        {
            build_min = char2int(attribute->value());
            attribute = attribute->next_attribute();
        }
        if (strcmp(attribute->name(), "build_max") == 0)
        {
            build_max = char2int(attribute->value());
        }

        //Download
        childroot = root->first_node("Download");
        if ((childroot == NULL))
            break;
        attribute = childroot->first_attribute();
        if (strcmp(attribute->name(), "appid") != 0)
            break;
        appid = char2int(attribute->value());
        attribute = attribute->next_attribute();
        if (strcmp(attribute->name(), "path") != 0)
            break;
        CString path(attribute->value());

        //Cloud
        childroot = root->first_node("Cloud");
        if ((childroot == NULL))
            break;
        attribute = childroot->first_attribute();
        if (strcmp(attribute->name(), "policy_id") != 0)
            break;
        policy_id = char2int(attribute->value());
        attribute = attribute->next_attribute();
        if (strcmp(attribute->name(), "switch_id") != 0)
            break;
        switch_id = char2int(attribute->value());
        attribute = attribute->next_attribute();
        if (strcmp(attribute->name(), "url_id") != 0)
            break;
        url_id = char2int(attribute->value());
        attribute = attribute->next_attribute();
        if (strcmp(attribute->name(), "md5_id") != 0)
            break;
        md5_id = char2int(attribute->value());

        //不需要去判断是否分配成功 会在调用函数中判断
        newItem = new CMapInfo(name, guid, path, build_min, build_max, appid, policy_id, switch_id, url_id, md5_id);
    } while (FALSE);

    return newItem;
}

CLocalMapParse::CLocalMapParse(CString FileName)
{
    m_bInited = XML_NONE;

    std::shared_ptr<char *> content;
    std::shared_ptr<char *>::element_type val;
    xml_document<> doc;
    xml_node<> *root;
    xml_attribute<> *attribute;
    int id = 0;

    if (!ReadXmlFile(FileName, content))
    {
        m_bInited = XML_FILE_NOT_EXIST;
        MY_ABORT;
        return;
    }

    try
    {
        val = *content;
        doc.parse<0>(val);
    }
    catch (const std::exception&)
    {
        m_bInited = XML_PARSE_ERR;
        MY_ABORT;
        return;
    }

    //解析Config配置
    root = doc.first_node();
    if (NULL == root)
    {
        m_bInited = XML_PARSE_ERR;
        MY_ABORT;
        return;
    }
    attribute = root->first_attribute();
    ConfigVersion = char2int(attribute->value());

    //CfgItem
    root = root->first_node("CfgItem");
    while (root)
    {
        CMapInfo *newItem = ParseDoc(root, id);
        if (newItem == NULL)
        {
            m_bInited = XML_MEMORY_ALLOC_FAIL;
            MY_ABORT;
            return;
        }

        LocalMap.insert(map<int, CMapInfo*>::value_type(id, newItem));
        root = root->next_sibling();
    }

    //根据已经创建好的LocalMap来创建PolicyIDMap
    CreatPolicyIDMap();

    m_bInited = XML_SUCCESS;
    return;
}

BOOL CLocalMapParse::GetConfigVersion(int & congfigversion)
{
    if (m_bInited < XML_NONE)
        return FALSE;

    ConfigVersion = this->ConfigVersion;
    return TRUE;
}

void CLocalMapParse::CreatPolicyIDMap()
{
    if (m_bInited < XML_NONE)
        return;

    map<int, CMapInfo*>::iterator Mapiter;
    map<int, std::vector<int>>::iterator Policyiter;
    int policy_id = 0;
    int id = 0;

    for (Mapiter = LocalMap.begin(); Mapiter != LocalMap.end(); Mapiter++)
    {
        id = Mapiter->first;
        Getpolicy_id(id, policy_id);

        Policyiter = PolicyIDMap.find(policy_id);
        if (Policyiter != PolicyIDMap.end())
        {
            Policyiter->second.push_back(id);
        }
        else
        {
            vector<int> temp;
            temp.push_back(id);
            PolicyIDMap.insert(map<int, vector<int>>::value_type(policy_id, temp));
        }
    }
}

//获取每个关键字的值
BOOL CLocalMapParse::Getname(int id, CString& name)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        name = iter->second->_Getname();
        return TRUE;
    }
    return FALSE;
}

BOOL CLocalMapParse::Getguid(int id, CString& guid)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        guid = iter->second->_Getguid();
        return TRUE;
    }
    return FALSE;
}

BOOL CLocalMapParse::Getpath(int id, CString& path)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        path = iter->second->_Getpath();
        return TRUE;
    }
    return FALSE;
}

BOOL CLocalMapParse::Getbuild_min(int id, int& build_min)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        build_min = iter->second->_Getbuild_min();
        return TRUE;
    }
    return FALSE;
}

BOOL CLocalMapParse::Getbuild_max(int id, int& build_max)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        build_max = iter->second->_Getbuild_max();
        return TRUE;
    }
    return FALSE;
}

BOOL CLocalMapParse::Getappid(int id, int& appid)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        appid = iter->second->_Getappid();
        return TRUE;
    }
    return FALSE;
}

BOOL CLocalMapParse::Getpolicy_id(int id, int& policy_id)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        policy_id = iter->second->_Getpolicy_id();
        return TRUE;
    }
    return FALSE;
}

BOOL CLocalMapParse::Getswitch_id(int id, int& switch_id)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        switch_id = iter->second->_Getswitch_id();
        return TRUE;
    }
    return FALSE;
}

BOOL CLocalMapParse::Geturl_id(int id, int& url_id)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        url_id = iter->second->_Geturl_id();
        return TRUE;
    }
    return FALSE;
}

BOOL CLocalMapParse::Getmd5_id(int id, int& md5_id)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);

    if (iter != LocalMap.end())
    {
        md5_id = iter->second->_Getmd5_id();
        return TRUE;
    }
    return FALSE;
}

const CMapInfo* CLocalMapParse::GetMapInfo(int id)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, CMapInfo*>::iterator iter = LocalMap.find(id);
    
    if (iter != LocalMap.end())
    {
        return iter->second;
    }
    return NULL;
}

BOOL CLocalMapParse::GetPolicyIDMapInfo(int policy_id, std::vector<int> &id)
{
    if (m_bInited != XML_SUCCESS)
        return FALSE;

    map<int, std::vector<int>>::iterator iter = PolicyIDMap.find(policy_id);

    if (iter != PolicyIDMap.end())
    {
        int size = iter->second.size();

        for (int i = 0; i < size; i++)
            id.push_back((iter->second)[i]);
        return TRUE;
    }

    return FALSE;
}

int CLocalMapParse::GetXMLErr()
{
    return m_bInited;
}