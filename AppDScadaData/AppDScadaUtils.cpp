#include "AppDScadaUtils.h"
#include "SysFuncCall.h"

SYS_CLASS_LOGGER_DEF(AppDScadaUtils);
SysDBSrvModelClient  AppDScadaUtils::s_dbClient;
ACE_Recursive_Thread_Mutex   AppDScadaUtils::s_guard;
std::map<SysString,AppDimDevInfo> AppDScadaUtils::s_dimDevInfoMap;
std::map<SysString,AppDevProtectInfo> AppDScadaUtils::s_devProInfoMap;
std::map<SysString,AppXHDevProtectInfo> AppDScadaUtils::s_XHdevProInfoMap;

SYS_HRESULT AppDScadaUtils::getDimDevInfoByDevCodeType( SysString &devCode, AppDimDevInfo *pDimDevInfo )
{
    SYS_HRESULT hr = SYS_OK;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, s_guard, NULL);
    if (!s_dimDevInfoMap.empty())
    {
        std::map<SysString,AppDimDevInfo>::iterator it = s_dimDevInfoMap.find(devCode);
        if (it != s_dimDevInfoMap.end())
        {
            *pDimDevInfo = it->second;
            return hr;
        }
    }
    SysString strSql = "select PROJECT_ID,PROJECT_NAME,AOR_ID,AOR_NAME,STATION_ID,STATION_NAME,TRANS_ID,TRANS_NAME,LINE_ID,LINE_NAME,\
                       OPER_USER,DEVICE_TYPE,POLE from DIM_DEVICE where DEVICE_CODE = '" + devCode + "'";
    std::vector<SysString> sqlVec;
    sqlVec.push_back(strSql);
    hr = s_dbClient.refreshMaster();
    SysDBModelSrvRsp devRsp;
    hr = s_dbClient.exeQuery(sqlVec,devRsp);
    if ( (SYS_OK != hr) || (devRsp.m_resultsVec.size() == 0))
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "获取通道数据失败. hr:" << hr);
        return SYS_FAIL;
    }
    SysInt devNum = devRsp.m_resultsVec[0].m_recordVec.size();
    if ( devRsp.m_resultsVec[0].m_error.length())
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "加载装置表失败：" << devRsp.m_resultsVec[0].m_error);
        return SYS_FAIL;
    }
    for (int i = 0; i < devNum; i++)
    {
        if(devRsp.m_resultsVec[0].m_recordVec[i].m_record.size() != 13)
        {
            SYS_LOG_MESSAGE(m_logger, ll_waring, "加载装置表失败：装置表数据列数量不等于13！");
            continue;
        }
        else
        {
            pDimDevInfo->m_strProID = devRsp.m_resultsVec[0].m_recordVec[i].m_record[0].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strProName = devRsp.m_resultsVec[0].m_recordVec[i].m_record[1].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strAorID = devRsp.m_resultsVec[0].m_recordVec[i].m_record[2].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strAorName = devRsp.m_resultsVec[0].m_recordVec[i].m_record[3].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strStationID = devRsp.m_resultsVec[0].m_recordVec[i].m_record[4].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strStationName = devRsp.m_resultsVec[0].m_recordVec[i].m_record[5].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strAreaID = devRsp.m_resultsVec[0].m_recordVec[i].m_record[6].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strAreaName = devRsp.m_resultsVec[0].m_recordVec[i].m_record[7].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strLineID = devRsp.m_resultsVec[0].m_recordVec[i].m_record[8].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strLineName = devRsp.m_resultsVec[0].m_recordVec[i].m_record[9].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strOperName = devRsp.m_resultsVec[0].m_recordVec[i].m_record[10].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strDevType = devRsp.m_resultsVec[0].m_recordVec[i].m_record[11].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strPole = devRsp.m_resultsVec[0].m_recordVec[i].m_record[12].m_dataValue.valueCharPtr;
            pDimDevInfo->m_strDevCode = devCode;
            s_dimDevInfoMap.insert(make_pair(pDimDevInfo->m_strDevCode,*pDimDevInfo));
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaUtils::getDevProtectInfoByDevCodeType( SysString &devCode, SysString &devType, AppDevProtectInfo *pDevProInfo )
{
    SYS_HRESULT hr = SYS_OK;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, s_guard, NULL);
    if (!s_devProInfoMap.empty())
    {
        std::map<SysString,AppDevProtectInfo>::iterator it = s_devProInfoMap.find(devCode);
        if (it != s_devProInfoMap.end())
        {
            *pDevProInfo = it->second;
            return hr;
        }
    }
    AppDevRelatedTableName tableNames;
    hr = getTableNameByDevType(devType,&tableNames);
    SysString strSql = "select SOFT_STRAP_I,CONST_VAL_I,DELAY_I,SOFT_STRAP_II,CONST_VAL_II,DELAY_II,SOFT_STRAP_III,CONST_VAL_III,DELAY_III,SOFT_STRAP_Z,\
                       CONST_VAL_Z,DELAY_Z from " + tableNames.m_strModelTable + " where DEVICE_CODE = '" + devCode + "'";
    std::vector<SysString> sqlVec;
    sqlVec.push_back(strSql);
    hr = s_dbClient.refreshMaster();
    SysDBModelSrvRsp devRsp;
    hr = s_dbClient.exeQuery(sqlVec,devRsp);
    if ( (SYS_OK != hr) || (devRsp.m_resultsVec.size() == 0))
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "获取通道数据失败. hr:" << hr);
        return SYS_FAIL;
    }
    SysInt devNum = devRsp.m_resultsVec[0].m_recordVec.size();
    if ( devRsp.m_resultsVec[0].m_error.length())
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "加载装置表失败：" << devRsp.m_resultsVec[0].m_error);
        return SYS_FAIL;
    }
    for (int i = 0; i < devNum; i++)
    {
        if(devRsp.m_resultsVec[0].m_recordVec[i].m_record.size() != 12)
        {
            SYS_LOG_MESSAGE(m_logger, ll_waring, "加载装置表失败：装置表数据列数量不等于12！");
            continue;
        }
        else
        {
            pDevProInfo->m_fISoftStrap = devRsp.m_resultsVec[0].m_recordVec[i].m_record[0].m_dataValue.valueDouble;
            pDevProInfo->m_fIConstValue = devRsp.m_resultsVec[0].m_recordVec[i].m_record[1].m_dataValue.valueDouble;
            pDevProInfo->m_fIDelay = devRsp.m_resultsVec[0].m_recordVec[i].m_record[2].m_dataValue.valueDouble;
            pDevProInfo->m_fIISoftStrap = devRsp.m_resultsVec[0].m_recordVec[i].m_record[3].m_dataValue.valueDouble;
            pDevProInfo->m_fIIConstValue = devRsp.m_resultsVec[0].m_recordVec[i].m_record[4].m_dataValue.valueDouble;
            pDevProInfo->m_fIIDelay = devRsp.m_resultsVec[0].m_recordVec[i].m_record[5].m_dataValue.valueDouble;
            pDevProInfo->m_fIIISoftStrap = devRsp.m_resultsVec[0].m_recordVec[i].m_record[6].m_dataValue.valueDouble;
            pDevProInfo->m_fIIIConstValue = devRsp.m_resultsVec[0].m_recordVec[i].m_record[7].m_dataValue.valueDouble;
            pDevProInfo->m_fIIIDelay = devRsp.m_resultsVec[0].m_recordVec[i].m_record[8].m_dataValue.valueDouble;
            pDevProInfo->m_fZeroSoftStrap = devRsp.m_resultsVec[0].m_recordVec[i].m_record[9].m_dataValue.valueDouble;
            pDevProInfo->m_fZeroConstValue = devRsp.m_resultsVec[0].m_recordVec[i].m_record[10].m_dataValue.valueDouble;
            pDevProInfo->m_fZeroDelay = devRsp.m_resultsVec[0].m_recordVec[i].m_record[11].m_dataValue.valueDouble;
            pDevProInfo->m_strDevCode = devCode;
            s_devProInfoMap.insert(make_pair(pDevProInfo->m_strDevCode,*pDevProInfo));
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaUtils::getFaultNameByType( SysUInt faultType, SysString& faultName, AppDScadaSignal* sig /*= NULL*/ )
{
    SYS_HRESULT hr = SYS_OK;
    faultType &= 0x00FFFFFF;
    switch(faultType)
    {
    case TAS_SHORT_WARNING_1:
        {
            if (NULL == sig)
            {
                faultName = " 发生速断故障.故障区段:";
            }
            else
            {
                if (sig->m_faultType == TAS_RECLOSING_FAILED)
                {
                    faultName = " 发生速断故障, 重合失败.故障区段:";
                }
                else if (sig->m_faultType == TAS_RECLOSING_SUCCESS)
                {
                    faultName = " 发生速断故障, 重合成功.故障区段:";
                }
                else
                {
                    faultName = " 发生速断故障.故障区段:";
                }
            }
            break;
        }
    case TAS_SHORT_WARNING_2:
    case TAS_SHORT_WARNING_3:
        {
            if (NULL == sig)
            {
                faultName = " 发生过流故障.故障区段:";
            }
            else
            {
                if (sig->m_faultType == TAS_RECLOSING_FAILED)
                {
                    faultName = " 发生过流故障, 重合失败.故障区段:";
                }
                else if (sig->m_faultType == TAS_RECLOSING_SUCCESS)
                {
                    faultName = " 发生过流故障, 重合成功.故障区段:";
                }
                else
                {
                    faultName = " 发生过流故障.故障区段:";
                }
            }
            break;
        }
    case TAS_GROUNDING_ALARM:
        {
            faultName = " 发生接地故障.故障区段:";
            break;
        }
    case TAS_TEMPORARY_GROUNDING_ALARM:
        {
            faultName = " 发生瞬时接地故障.故障区段:";
            break;
        }
    case TAS_GROUNDING_DISAPPEAR:
        {
            break;
        }
    case TAS_RECLOSING_SUCCESS:
        {
            break;
        }
    default:
        {
            faultName = "未知故障类型.故障区段:";
            break;
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaUtils::getTableNameByDevType( SysString &strDevType,AppDevRelatedTableName *tablePtr )
{
    SYS_HRESULT hr = SYS_OK;
    if (XHQD_520 == strDevType)
    {
        tablePtr->m_strHisTable = "FACT_DEVICE_520_HISTORY";
        tablePtr->m_strModelTable = "DIM_DEVICE_520_DZ";
        tablePtr->m_strProtocolTable = "DIM_PROTOCOL_104_520";
        tablePtr->m_strRtTable = "FACT_DEVICE_520_MONTIOR";
    }
    else if (XHQD_120 == strDevType)
    {
        tablePtr->m_strHisTable = "FACT_DEVICE_DROP_HISTORY";
        tablePtr->m_strModelTable = "DIM_DEVICE_DROP_DZ";
        tablePtr->m_strProtocolTable = "DIM_PROTOCOL_DROP";
        tablePtr->m_strRtTable = "FACT_DEVICE_DROP_MONITOR";
    }
    else if (XHQD_521_1 == strDevType)
    {
        tablePtr->m_strHisTable = "FACT_DEVICE_521_1_HISTORY";
        tablePtr->m_strModelTable = "DIM_DEVICE_521_1_DZ";
        tablePtr->m_strProtocolTable = "DIM_PROTOCOL_104_520_1";
        tablePtr->m_strRtTable = "FACT_DEVICE_521_1_MONTIOR";
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "获取设备相关表名失败,设备类型:" << strDevType);
        hr = SYS_FAIL;
    }
    return hr;
}

SYS_HRESULT AppDScadaUtils::getXHDevProtectInfoByDevCodeType( SysString &devCode, SysString &devType, AppXHDevProtectInfo *pXHDevProInfo )
{
    SYS_HRESULT hr = SYS_OK;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, s_guard, NULL);
    if (!s_XHdevProInfoMap.empty())
    {
        std::map<SysString,AppXHDevProtectInfo>::iterator it = s_XHdevProInfoMap.find(devCode);
        if (it != s_XHdevProInfoMap.end())
        {
            *pXHDevProInfo = it->second;
            return hr;
        }
    }
    AppDevRelatedTableName tableNames;
    hr = getTableNameByDevType(devType,&tableNames);
    SysString strSql = "select SOFT_STRAP_I,CONST_VAL_I,DELAY_I,SOFT_STRAP_II,CONST_VAL_II,DELAY_II,SOFT_STRAP_III,\
                        from " + tableNames.m_strModelTable + " where DEVICE_CODE = '" + devCode + "'";
    std::vector<SysString> sqlVec;
    sqlVec.push_back(strSql);
    hr = s_dbClient.refreshMaster();
    SysDBModelSrvRsp devRsp;
    hr = s_dbClient.exeQuery(sqlVec,devRsp);
    if ( (SYS_OK != hr) || (devRsp.m_resultsVec.size() == 0))
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "获取通道数据失败. hr:" << hr);
        return SYS_FAIL;
    }
    SysInt devNum = devRsp.m_resultsVec[0].m_recordVec.size();
    if ( devRsp.m_resultsVec[0].m_error.length())
    {
        SYS_LOG_MESSAGE(m_logger, ll_waring, "加载装置表失败：" << devRsp.m_resultsVec[0].m_error);
        return SYS_FAIL;
    }
    for (int i = 0; i < devNum; i++)
    {
        if(devRsp.m_resultsVec[0].m_recordVec[i].m_record.size() != 12)
        {
            SYS_LOG_MESSAGE(m_logger, ll_waring, "加载装置表失败：装置表数据列数量不等于12！");
            continue;
        }
        else
        {
            pXHDevProInfo->m_iQuickBreakProtectThreshold = devRsp.m_resultsVec[0].m_recordVec[i].m_record[0].m_dataValue.valueInt;
            pXHDevProInfo->m_iQuickBreakTrip = devRsp.m_resultsVec[0].m_recordVec[i].m_record[1].m_dataValue.valueInt;
            pXHDevProInfo->m_iQuickBreakDelay = devRsp.m_resultsVec[0].m_recordVec[i].m_record[2].m_dataValue.valueInt;
            pXHDevProInfo->m_iOverCurrentProtectThreshold = devRsp.m_resultsVec[0].m_recordVec[i].m_record[3].m_dataValue.valueInt;
            pXHDevProInfo->m_iOverCurrentTrip = devRsp.m_resultsVec[0].m_recordVec[i].m_record[4].m_dataValue.valueInt;
            pXHDevProInfo->m_iOverCurrentDelay = devRsp.m_resultsVec[0].m_recordVec[i].m_record[5].m_dataValue.valueInt;
            pXHDevProInfo->m_iZeroCurrentThreshold = devRsp.m_resultsVec[0].m_recordVec[i].m_record[6].m_dataValue.valueInt;
            pXHDevProInfo->m_strDevCode = devCode;
            s_XHdevProInfoMap.insert(make_pair(pXHDevProInfo->m_strDevCode,*pXHDevProInfo));
        }
    }
    return hr;
}
