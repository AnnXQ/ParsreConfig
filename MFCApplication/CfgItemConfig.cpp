#include "stdafx.h"
#include "CfgItemConfig.h"
#include "JsonParse.h"
#include <atlpath.h>


// JSON 配置节点
#define CFGITEM_CONFIG_ROOT                     "Config"
#define CFGITEM_CONFIG_VER                      "version"

#define CFGITEM_CONFIG_CFGITEM                  "CfgItem"
#define CFGITEM_CONFIG_CFGITEM_ATTR_ID          "id"
#define CFGITEM_CONFIG_CFGITEM_ATTR_NAME        "name"
#define CFGITEM_CONFIG_CFGITEM_ATTR_GUID        "guid"

#define CFGITEM_CONFIG_CFGITEM_QQPC             "QQPCMgrSupport"
#define CFGITEM_CONFIG_CFGITEM_QQPC_ATTR_MIN    "build_min"
#define CFGITEM_CONFIG_CFGITEM_QQPC_ATTR_MAX    "build_max"

#define CFGITEM_CONFIG_CFGITEM_DOWN             "Download"
#define CFGITEM_CONFIG_CFGITEM_DOWN_ATTR_APPID  "appid"
#define CFGITEM_CONFIG_CFGITEM_DOWN_ATTR_PATH   "path"

#define CFGITEM_CONFIG_CFGITEM_CLOUD            "Cloud"
#define CFGITEM_CONFIG_CFGITEM_CLOUD_ATTR_PID   "policy_id"
#define CFGITEM_CONFIG_CFGITEM_CLOUD_ATTR_SID   "switch_id"
#define CFGITEM_CONFIG_CFGITEM_CLOUD_ATTR_UID   "url_id"
#define CFGITEM_CONFIG_CFGITEM_CLOUD_ATTR_MID   "md5_id"


CCfgItemConfig::CCfgItemConfig()
    :m_dwVersion(0)
{
}


CCfgItemConfig::~CCfgItemConfig()
{
}

BOOL CCfgItemConfig::Load()
{
#define CFGITEM_CONFIG_FILE_PATH_NAME           _T("LocalConfig.json")
    
    BOOL bRet = FALSE;

    do
    {
        ATL::CAtlString strFilePath = CFGITEM_CONFIG_FILE_PATH_NAME;
        if (!ATLPath::FileExists(strFilePath))
        {
            CAtlString strCfgDir;
            GetModuleFileName(NULL, strCfgDir.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
            strCfgDir.ReleaseBuffer();
            PathRemoveFileSpec(strCfgDir.GetBuffer());
            strCfgDir.ReleaseBuffer();
            if (!strCfgDir.IsEmpty() && (strCfgDir.Right(1) != L'\\' || strCfgDir.Right(1) != L'/'))
            {
                strCfgDir += L"\\";
            }

            strFilePath = strCfgDir + CFGITEM_CONFIG_FILE_PATH_NAME;
            if (!ATLPath::FileExists(strFilePath))
            {
                break;
            }
        }

        CJsonParse<> jsonParse;
        if (!jsonParse.Load(strFilePath))
            break;

        Json::Value& json_root = jsonParse.GetRoot();
        if (json_root.isNull())
            break;

        Json::Value json_config = json_root.get(CFGITEM_CONFIG_ROOT, NULL);
        if (json_config.isNull())
            break;

        Json::Value json_version = json_config.get(CFGITEM_CONFIG_VER, NULL);
        if (json_version.isNull())
            break;
        m_dwVersion = json_version.asUInt();

        Json::Value json_cfgitem = json_config.get(CFGITEM_CONFIG_CFGITEM, NULL);
        if (json_cfgitem.isNull())
            break;

        int arrSize = json_cfgitem.size();
        for (int i = 0; i < arrSize; i++)
        {
            Json::Value& json_temp = json_cfgitem[i];
            if (json_temp.isNull())
                break;

            CFGITEM cfgItem;
            cfgItem.id = json_temp.get(CFGITEM_CONFIG_CFGITEM_ATTR_ID, 0).asUInt();
            cfgItem.name = json_temp.get(CFGITEM_CONFIG_CFGITEM_ATTR_NAME, _T("")).asCString();
            cfgItem.guid = json_temp.get(CFGITEM_CONFIG_CFGITEM_ATTR_GUID, _T("")).asCString();

            Json::Value json_support = json_temp.get(CFGITEM_CONFIG_CFGITEM_QQPC, NULL);
            if (!json_support.isNull())
            {
                cfgItem.QQPcMgrSupport.build_min = json_support.get(CFGITEM_CONFIG_CFGITEM_QQPC_ATTR_MIN, 0).asUInt();
                cfgItem.QQPcMgrSupport.build_max = json_support.get(CFGITEM_CONFIG_CFGITEM_QQPC_ATTR_MAX, 0).asUInt();
            }

            Json::Value json_download = json_temp.get(CFGITEM_CONFIG_CFGITEM_DOWN, NULL);
            if (!json_download.isNull())
            {
                cfgItem.Download.appid = json_download.get(CFGITEM_CONFIG_CFGITEM_DOWN_ATTR_APPID, 0).asUInt();
                cfgItem.Download.path = json_download.get(CFGITEM_CONFIG_CFGITEM_DOWN_ATTR_PATH, _T("")).asCString();
            }

            Json::Value json_cloud = json_temp.get(CFGITEM_CONFIG_CFGITEM_CLOUD, NULL);
            if (!json_cloud.isNull())
            {
                cfgItem.Cloud.policy_id = json_cloud.get(CFGITEM_CONFIG_CFGITEM_CLOUD_ATTR_PID, 0).asUInt();
                cfgItem.Cloud.switch_id = json_cloud.get(CFGITEM_CONFIG_CFGITEM_CLOUD_ATTR_SID, 0).asUInt();
                cfgItem.Cloud.url_id = json_cloud.get(CFGITEM_CONFIG_CFGITEM_CLOUD_ATTR_UID, 0).asUInt();
                cfgItem.Cloud.md5_id = json_cloud.get(CFGITEM_CONFIG_CFGITEM_CLOUD_ATTR_MID, 0).asUInt();

            }
            m_vecCfgItem.push_back(cfgItem);
        }

        bRet = CreatPolicyIDMap();
    } while (FALSE);

    return bRet;
}

void CCfgItemConfig::UnLoad()
{
    m_vecCfgItem.clear();
}

BOOL CCfgItemConfig::CreatPolicyIDMap()
{
    BOOL bRet = FALSE;

    do
    {
        std::map<UINT, std::vector<UINT>>::iterator Mapiter;

        UINT policy_id = 0;
        UINT id = 0;
        int vecSize = m_vecCfgItem.size();
        if (vecSize <= 0)
            break;

        for (int i = 0; i < vecSize; i++)
        {
            id = m_vecCfgItem[i].id;
            policy_id = m_vecCfgItem[i].Cloud.policy_id;

            Mapiter = m_mapPolicyID.find(policy_id);
            if (Mapiter != m_mapPolicyID.end())
            {
                Mapiter->second.push_back(id);
            }
            else
            {
                std::vector<UINT> temp;
                temp.push_back(id);
                m_mapPolicyID.insert(std::map<UINT, std::vector<UINT>>::value_type(policy_id, temp));
            }
        }

        bRet = TRUE;

    } while (FALSE);

    return bRet;
}

int CCfgItemConfig::GetVersion()
{
    return m_dwVersion;
}

BOOL CCfgItemConfig::GetCfgItem(UINT uIndex, CCfgItemConfig::CFGITEM& vecCfgItem)
{
    BOOL bRet = FALSE;

    //这里假设的id是以0开始的连续数字
    if (uIndex < m_vecCfgItem.size())
    {
        vecCfgItem = m_vecCfgItem[uIndex];
        bRet = TRUE;
    }

    return bRet;
}

BOOL CCfgItemConfig::GetIDGroup(UINT uIndex, std::vector<UINT>& idGroup)
{
    std::map<UINT, std::vector<UINT>>::iterator iter = m_mapPolicyID.find(uIndex);

    if (iter != m_mapPolicyID.end())
    {
        int size = iter->second.size();

        for (int i = 0; i < size; i++)
            idGroup.push_back((iter->second)[i]);
        return TRUE;
    }

    return FALSE;
}
