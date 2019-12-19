
#include        "AppDScadaFL4Tiny.h"
#include        "SysFuncCall.h"
#include        "SysRtdbmsTableOperator.h"
#include        "AppDScadaFLConfig.h"
#include        "AppDScadaCallDataWorker.h"
#include        "SysServiceMonitorClient.h"


SYS_CLASS_LOGGER_DEF(AppDScadaTinyWorker);
SYS_CLASS_LOGGER_DEF(AppTinyFault);
AppDScadaMsgQueue<AppDScadaSignal> AppDScadaTinyWorker::m_queue;
const SysInt     DEF_OVER_CURRENT_MAX = 300;
/************************************************************************/
/*     AppDScadaTinyWorker                                                          
/************************************************************************/
SYS_HRESULT AppDScadaTinyWorker::init()
{
    SYS_HRESULT hr = SYS_OK;
    m_pModelDB = new SysDBSrvModelClient();
    m_pModelDB->refreshMaster();
    m_queue.setMode(AppDScadaMsgQueue<AppDScadaSignal>::NONE_BLOCK_MODE);
    AppDScadaSignal* sigPtr = new AppDScadaSignal;
    SysULong presentTime = SysOSUtils::getTimeNowMsec() - AppTinyFault::OVERLOAD_DATA_INVERVAL_TIME;
    SysChar presentTimec[15];
    sprintf(presentTimec,"%lld",presentTime);
    SysString presentTimes = presentTimec;
    getMsgDatabase(presentTimes);
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::run()
{
    SYS_HRESULT hr = SYS_OK;
    SYS_LOG_MESSAGE(m_logger, ll_debug, "0.4kV系统故障判定服务启动...");
    // 50 milliseconds
    ACE_Time_Value interval(0, 50 * 1000); // 
    SysUInt counter = 1;
    while(true)
    {
        ACE_OS::sleep(interval);
        counter ++;
        AppDScadaSignal* sigPtr = NULL;
        m_queue.getQ(sigPtr);

        if (NULL != sigPtr)
        {
            if(sigPtr->m_faultType == 58)
            {
                sigPtr->m_dataOrigin = 888888;
                sigPtr->m_faultType = 54;
            }
            else
            {
                sigPtr->m_dataOrigin = sigPtr->m_occurDevTime;
                AppDataPersisitPkg * persistPgPtr = new AppDataPersisitPkg;
                assert(NULL != persistPgPtr);
                if(SYS_OK == (hr = AppDScadaDataToDBWorker::doePutMontior(sigPtr,persistPgPtr)))
                {
                    for(SysInt i = 0;i < persistPgPtr->sqlVec.size() ;i++)
                    {
                        SYS_LOG_MESSAGE(m_logger, ll_debug, "入库内容"<< persistPgPtr->sqlVec[i]);
                    }
                    AppTriggerSampleQueue::getInstance()->putQ(persistPgPtr);
                }
            }
            SYS_LOG_MESSAGE(m_logger, ll_debug, "接收到一个信号故障类型："<< sigPtr->m_faultType);//sigPtr->m_faultType
            processProtectionSig(sigPtr);
            delete sigPtr;
            sigPtr = NULL;
        }

        if (0 == counter % 100) // 5 seconds.
        {
            SYS_MONITOR_UPDATE_PROBE(1);
            updateSCMsgMap();

            updateCSendMap();

            updateLostVoltageMsgMap();

            updateInComingtMsgMap();

            devNormalTest();
        }
    }

    SYS_LOG_MESSAGE(m_logger, ll_debug, "0.4kV系统故障判定服务退出...");
    return hr;
}



SYS_HRESULT AppDScadaTinyWorker::updateSCMsgMap()
{
    SYS_HRESULT hr = SYS_OK;
    SysULong now = SysOSUtils::getTimeNowMsec();
    GLOBAL_TINY_FAULT_MAP_IT index;
    for (index = m_curSCMsgMap.begin(); index != m_curSCMsgMap.end();)
    {
        if((index->second.m_ftypeID == 23)&&(((now - (index->second.m_putStoragetTime)) > (AppTinyFault::SERIOUS_OVERLOAD_DATA_INVERVAL_TIME))))
        {
            m_curSCMsgMap[index->first] = index->second;
            m_curSCMsgMap.erase(index++);
        }
        else if((index->second.m_ftypeID == 24)&&((now - (index->second.m_putStoragetTime)) > (AppTinyFault::OVERLOAD_DATA_INVERVAL_TIME)))
        {
            m_curSCMsgMap[index->first] = index->second;
            m_curSCMsgMap.erase(index++);
        }
        else
        {
            index++;
        }
    }
    return hr;
}


SYS_HRESULT AppDScadaTinyWorker::updateCSendMap()
{
    SYS_HRESULT hr = SYS_OK;
    GLOBAL_TINY_FAULT_MAP_IT index;
    for (index = m_curSCSendMap.begin(); index != m_curSCSendMap.end();)
    {
         SYS_LOG_MESSAGE(m_logger, ll_debug, "updateCSendMap 台区id"<<index->second.m_transID <<"故障类型" << index->second.m_ftypeID << "故障数："<< index->second.m_ftypeDuration);
         SysString transid = index->second.m_transID;
         SysInt ftypeID = index->second.m_ftypeID;
         SysInt ftypeDuration = index->second.m_ftypeDuration;
        GLOBAL_TINY_FAULT_MAP_IT index1 = m_curSCMsgMap.find(transid);
        if(index1 != m_curSCMsgMap.end())
        {
            if((m_curSCMsgMap[transid].m_ftypeID == 24)&&(ftypeID == 23)&& (ftypeDuration >= 2))
            {
                m_curSCMsgSendMap[transid].m_faultType = 23;
                SYS_LOG_MESSAGE(m_logger, ll_debug, "装置编号"<<m_curSCMsgSendMap[transid].m_devCode << "台区id"<<transid);//sigPtr->m_faultType
                overloadPutMsg(transid);
                return hr;
            }
            index++;
        }
        else
        {
            if((ftypeID == 23)&& (ftypeDuration >= 2))
            {
                m_curSCMsgSendMap[transid].m_faultType = 23;
                SYS_LOG_MESSAGE(m_logger, ll_debug, "装置编号"<<m_curSCMsgSendMap[transid].m_devCode << "台区id"<<transid);//sigPtr->m_faultType
                overloadPutMsg(transid);
                return hr;
            }
            else if((ftypeID == 24)&& (ftypeDuration >= 6))
            {
                m_curSCMsgSendMap[transid].m_faultType = 24;
                SYS_LOG_MESSAGE(m_logger, ll_debug, "装置编号"<<m_curSCMsgSendMap[transid].m_devCode << "台区id"<<transid);//sigPtr->m_faultType
                overloadPutMsg(transid);
                return hr;
            }
            index++;
        }
    }
    return hr;
}


SYS_HRESULT AppDScadaTinyWorker::overloadPutMsg(SysString & transID)
{
    SYS_HRESULT hr = SYS_OK;
    GLOBAL_TINY_FAULT_MAP_IT index1;
    SysString devID = m_curSCMsgSendMap[transID].m_devCode;
    SYS_LOG_MESSAGE(m_logger, ll_debug, "装置编号"<< transID<< "台区id"<<devID);//sigPtr->m_faultType
    for (index1 = m_curSCSendMap.begin(); index1 != m_curSCSendMap.end();)
    {
        if((index1->second.m_transID == transID)&&(index1->second.m_ftypeID == m_curSCMsgSendMap[transID].m_faultType))
        {
            if((index1->second.m_ftypeID == 23)&& (index1->second.m_ftypeDuration >= 2))
            {
                m_curSCSendMap[devID].devCode.push_back(index1->second.m_deviceCode);
                m_curSCSendMap[devID].pole.push_back(index1->second.m_pole);
                m_curSCSendMap[devID].currenta.push_back(index1->second.m_currenta);
                m_curSCSendMap[devID].phase.push_back(index1->second.m_phase);
                m_curSCSendMap[devID].devTime.push_back(index1->second.m_updateDevTime);
            }
            else if((index1->second.m_ftypeID == 24)&& (index1->second.m_ftypeDuration >= 6))
            {
                m_curSCSendMap[devID].devCode.push_back(index1->second.m_deviceCode);
                m_curSCSendMap[devID].pole.push_back(index1->second.m_pole);
                m_curSCSendMap[devID].currenta.push_back(index1->second.m_currenta);
                m_curSCSendMap[devID].phase.push_back(index1->second.m_phase);
                m_curSCSendMap[devID].devTime.push_back(index1->second.m_updateDevTime);
            }
        }
        index1++;
    }
    refreshDataAndReprotMsg(&m_curSCMsgSendMap[transID]);
    AppTinyFault* newFault = new AppTinyFault;
    putNewFault(&m_curSCMsgSendMap[transID],newFault);
    newFault->m_putStoragetTime = SysOSUtils::getTimeNowMsec();
    m_curSCMsgMap[transID] = *newFault;
    delete newFault;
    GLOBAL_TINY_FAULT_MAP_IT index = m_curSCSendMap.begin();
    SysString transid = transID;
    while (index != m_curSCSendMap.end())
    {
        if(index->second.m_transID == transid)
        {
            m_curSCSendMap[index->first] = index->second;
            m_curSCSendMap.erase(index++);
            
        }
        else
        {
            index++;
        }
    }
    m_curSCMsgSendMap.erase(transid);
    return hr;
}


SYS_HRESULT AppDScadaTinyWorker::updateLostVoltageMsgMap()
{
    SYS_HRESULT hr = SYS_OK;

    SysULong now = SysOSUtils::getTimeNowMsec();
    GLOBAL_TINY_FAULT_MAP_IT index;
    for (index = m_curLostVoltageMsgMap.begin(); index != m_curLostVoltageMsgMap.end();)
    {
        if(((now - index->second.m_updateDevTime) > 120000)&&(index->second.m_ftypeDuration == 0))
        {
            refreshDataAndReprotMsg(&m_curLostVoltageMap[index->second.m_transID]);
            index->second.m_putStoragetTime = now;
            index->second.m_ftypeDuration = 1;
            m_curLostVoltageMap.erase(index->second.m_transID);

        }
        if((((now - (index->second.m_putStoragetTime))) > (AppTinyFault::LOST_VOLTAGE_DATA_INVERVAL_TIME)))
        {
            m_curLostVoltageMsgMap[index->first] = index->second;
            m_curLostVoltageMsgMap.erase(index++);
        }
        else
        {
            index++;
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::updateInComingtMsgMap()
{
    SYS_HRESULT hr = SYS_OK;


    SysULong now = SysOSUtils::getTimeNowMsec();
    GLOBAL_TINY_FAULT_MAP_IT index;
    for (index = m_curInComingtMsgMap.begin(); index != m_curInComingtMsgMap.end();)
    {
        if(((now - index->second.m_updateDevTime) > 120000)&&(index->second.m_ftypeDuration == 0))
        {
            refreshDataAndReprotMsg(&m_curInComingtMap[index->second.m_transID]);
            index->second.m_putStoragetTime = now;
            index->second.m_ftypeDuration = 1;
            m_curInComingtMap.erase(index->second.m_transID);
            
        }
        if(index->second.m_ftypeDuration == 1)
        {
            GLOBAL_TINY_FAULT_MAP_IT index1 = m_curLostVoltageMsgMap.find(index->second.m_transID);
            if(index1 != m_curLostVoltageMsgMap.end())
            {
                index1->second.m_putStoragetTime = (now - AppTinyFault::LOST_VOLTAGE_DATA_INVERVAL_TIME + 480000);
            }
        }
        if((index->second.m_ftypeDuration == 1)&&((now - index->second.m_updateDevTime) > 240000))
        {
            m_curInComingtMsgMap[index->first] = index->second;
            m_curInComingtMsgMap.erase(index++);
        }
        else
        {
            index++;
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::processProtectionSig(AppDScadaSignal* sigPtr)
{
	SYS_HRESULT hr = SYS_OK;
    assert(NULL != sigPtr);
    SYS_LOG_MESSAGE(m_logger, ll_debug, "接收到设备信号 设备："<< sigPtr->m_devCode << "故障类型：" <<sigPtr->m_faultType);
    SysString statusValue;
    if(SYS_OK == (hr = getDeviceStatus(sigPtr->m_devCode,statusValue)))
    {
        const SysChar *buf = statusValue.c_str();
        if(((buf[0]&0x0f)!= 0)&&((buf[1]&0x0f)!= 0)&&((buf[2]&0x0f)!= 0))
        {
            SYS_LOG_MESSAGE(m_logger, ll_debug,"装置已拆除、退出运行、或者离线："<<sigPtr->m_devCode);
        }
        else
        {
            if ((sigPtr->isSCFault()))
            {
                if( (buf[3]&0x0f)== 0)
                {
                    processSCSignal(sigPtr);///过载信息
                }
                else
                {
                    SYS_LOG_MESSAGE(m_logger, ll_debug,"装置电流异常："<<sigPtr->m_devCode);
                }
            }
            else if((sigPtr->m_faultType == AppTinyFault::GROUNDING))
            {
                if( (buf[3]&0x0f)== 0)
                {
                    SYS_LOG_MESSAGE(m_logger, ll_debug, "接收到一个接地故障 设备："<< sigPtr->m_devCode);
                }
                else
                {
                    SYS_LOG_MESSAGE(m_logger, ll_debug,"装置电流异常："<<sigPtr->m_devCode);
                }
                
            }
            else if((sigPtr->m_faultType==AppTinyFault::LOST_VOLTAGE))
            {
                if( (buf[4]&0x0f)== 0)
                {
                    processLostVoltageSignal(sigPtr);//失压故障
                }
                else
                {
                    SYS_LOG_MESSAGE(m_logger, ll_debug,"装置电压异常："<<sigPtr->m_devCode);
                }
            }
            else if((sigPtr->m_faultType==AppTinyFault::IN_COMINGT))
            {
                if( (buf[4]&0x0f)== 0)
                {
                    processInComingtSignal(sigPtr);//电压恢复
                }
                else
                {
                    SYS_LOG_MESSAGE(m_logger, ll_debug,"装置电压异常："<<sigPtr->m_devCode);
                }
            }
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::processSCSignal(AppDScadaSignal* sigPtr)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_LOG_MESSAGE(m_logger, ll_debug, "开始处理过负荷信息 设备："<< sigPtr->m_devCode << "速断定值"<<(sigPtr->m_QuickBreakLimit * 1.5));
    AppTinyFault* newFault = new AppTinyFault;
    GLOBAL_TINY_FAULT_MAP_IT index;
    refreshSCMsgMap(sigPtr,newFault);
    delete newFault;
    return hr;
}



SYS_HRESULT AppDScadaTinyWorker::processLostVoltageSignal(AppDScadaSignal* sigPtr)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_LOG_MESSAGE(m_logger, ll_debug, "开始处理失压信息 设备："<< sigPtr->m_devCode);
    AppTinyFault* newFault = new AppTinyFault;
    refreshLostVoltageMsgMap(sigPtr,newFault);
    delete newFault;
    return hr;
}


SYS_HRESULT AppDScadaTinyWorker::processInComingtSignal(AppDScadaSignal* sigPtr)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_LOG_MESSAGE(m_logger, ll_debug, "开始处理恢复电压信息 设备："<< sigPtr->m_devCode);
    AppTinyFault* newFault = new AppTinyFault;
    refreshInComingtMsgMap(sigPtr,newFault);
    delete newFault;
    return hr;
}



SYS_HRESULT AppDScadaTinyWorker::refreshLostVoltageMsgMap(AppDScadaSignal* sigPtr,AppTinyFault* newFault)
{
    SYS_HRESULT hr = SYS_OK;
    SysInt install = 11;
    for(SysInt i = 0 ; i < 3; i++)
    {
        if((sigPtr->m_areaID.size() <= 0)||(sigPtr->m_pole.size() <= 0) || (install == 11))
        {
            getTransID(sigPtr,&install);
        }
    }
    GLOBAL_TINY_FAULT_MAP_IT index = m_curLostVoltageMsgMap.find(sigPtr->m_areaID);
    if(index != m_curLostVoltageMsgMap.end())
    {
        SysInt nRnt = std::count(m_curLostVoltageMsgMap[sigPtr->m_areaID].devCode.begin(),m_curLostVoltageMsgMap[sigPtr->m_areaID].devCode.end(),sigPtr->m_devCode);
        if(nRnt == SYS_OK)
        {
        m_curLostVoltageMsgMap[sigPtr->m_areaID].devCode.push_back(sigPtr->m_devCode);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].pole.push_back(sigPtr->m_pole);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].currenta.push_back(sigPtr->m_value[0]);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].phase.push_back(sigPtr->m_phase);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].install.push_back(install);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].devTime.push_back(sigPtr->m_dataOrigin);
        SYS_LOG_MESSAGE(m_logger, ll_debug, "台区接收一个失压信号 台区："<< sigPtr->m_areaID << sigPtr->m_pole);
    }
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_debug, "台区接收一个失压信号 台区："<< sigPtr->m_areaID << sigPtr->m_pole);
        putNewFault(sigPtr,newFault);
        m_curLostVoltageMsgMap[sigPtr->m_areaID] = *newFault;
        m_curLostVoltageMsgMap[sigPtr->m_areaID].devCode.push_back(sigPtr->m_devCode);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].pole.push_back(sigPtr->m_pole);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].currenta.push_back(sigPtr->m_value[0]);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].phase.push_back(sigPtr->m_phase);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].install.push_back(install);
        m_curLostVoltageMsgMap[sigPtr->m_areaID].devTime.push_back(sigPtr->m_dataOrigin);
        m_curLostVoltageMap[sigPtr->m_areaID] = *sigPtr;
    }
    return hr;
}


SYS_HRESULT AppDScadaTinyWorker::refreshInComingtMsgMap(AppDScadaSignal* sigPtr,AppTinyFault* newFault)
{
    SYS_HRESULT hr = SYS_OK;
    SysInt install = 11;
    getTransID(sigPtr,&install);
    GLOBAL_TINY_FAULT_MAP_IT index = m_curInComingtMsgMap.find(sigPtr->m_areaID);
    if(index != m_curInComingtMsgMap.end())
    {
        SysInt nRnt = std::count(m_curInComingtMsgMap[sigPtr->m_areaID].devCode.begin(),m_curInComingtMsgMap[sigPtr->m_areaID].devCode.end(),sigPtr->m_devCode);
        if(nRnt == SYS_OK)
        {
        m_curInComingtMsgMap[sigPtr->m_areaID].devCode.push_back(sigPtr->m_devCode);
        m_curInComingtMsgMap[sigPtr->m_areaID].pole.push_back(sigPtr->m_pole);
        m_curInComingtMsgMap[sigPtr->m_areaID].currenta.push_back(sigPtr->m_value[0]);
        m_curInComingtMsgMap[sigPtr->m_areaID].phase.push_back(sigPtr->m_phase);
        m_curInComingtMsgMap[sigPtr->m_areaID].install.push_back(install);
        m_curInComingtMsgMap[sigPtr->m_areaID].devTime.push_back(sigPtr->m_occurDevTime);
        SYS_LOG_MESSAGE(m_logger, ll_debug, "台区接收一个来电信号 台区："<< sigPtr->m_areaID <<"装置" << sigPtr->m_pole);
    }
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_debug, "台区接收一个来电信号 台区："<< sigPtr->m_areaID << "装置" << sigPtr->m_pole);
        putNewFault(sigPtr,newFault);
        m_curInComingtMsgMap[sigPtr->m_areaID] = *newFault;
        m_curInComingtMsgMap[sigPtr->m_areaID].devCode.push_back(sigPtr->m_devCode);
        m_curInComingtMsgMap[sigPtr->m_areaID].pole.push_back(sigPtr->m_pole);
        m_curInComingtMsgMap[sigPtr->m_areaID].currenta.push_back(sigPtr->m_value[0]);
        m_curInComingtMsgMap[sigPtr->m_areaID].phase.push_back(sigPtr->m_phase);
        m_curInComingtMsgMap[sigPtr->m_areaID].install.push_back(install);
        m_curInComingtMsgMap[sigPtr->m_areaID].devTime.push_back(sigPtr->m_occurDevTime);
        m_curInComingtMap[sigPtr->m_areaID] = *sigPtr;
    }
    return hr;
}


SYS_HRESULT AppDScadaTinyWorker::refreshSCMsgMap(AppDScadaSignal* sigPtr,AppTinyFault* newFault)
{
    SYS_HRESULT hr = SYS_OK;
    GLOBAL_TINY_FAULT_MAP_IT index = m_curSCMsgMap.find(sigPtr->m_areaID);
    GLOBAL_TINY_FAULT_MAP_IT index1 = m_curSCSendMap.find(sigPtr->m_devCode);
    GLOBAL_TINY_MSG_MAP_IT msg_index =  m_curSCMsgSendMap.find(sigPtr->m_areaID);

    SysULong now = SysOSUtils::getTimeNowMsec() - sigPtr->m_occurDevTime;
    if(index != m_curSCMsgMap.end())
    {
        if(m_curSCMsgMap[sigPtr->m_areaID].m_ftypeID == 23)
        {
            return hr;
        }
        if((m_curSCMsgMap[sigPtr->m_areaID].m_ftypeID == 24)&&(sigPtr->m_faultType == 24))
        {
            return hr;
        }
    }
    if(index1 != m_curSCSendMap.end())
    {
        SysULong now = sigPtr->m_occurDevTime - m_curSCSendMap[sigPtr->m_devCode].m_putStoragetTime ;
        if(now > 360000)
        {
            SYS_LOG_MESSAGE(m_logger, ll_debug, "故障上发时差 ："<< now/1000 << "故障时戳"<<sigPtr->m_occurDevTime << "上次故障时间" <<m_curSCSendMap[sigPtr->m_devCode].m_putStoragetTime);
            putNewFault(sigPtr,newFault);
            newFault->m_ftypeDuration = 1;
            m_curSCSendMap[sigPtr->m_devCode] = *newFault;
            if(msg_index == m_curSCMsgSendMap.end())
            {
                m_curSCMsgSendMap[sigPtr->m_areaID] = *sigPtr;
                SYS_LOG_MESSAGE(m_logger, ll_debug, "台区第一个故障 台区 ："<< sigPtr->m_areaID << "code ："<<sigPtr->m_devCode);
            }
        }
        else
        {
            if((m_curSCSendMap[sigPtr->m_devCode].m_ftypeID == 24) && (sigPtr->m_faultType == 23))
            {
                putNewFault(sigPtr,newFault);
                newFault->m_ftypeDuration = 1;
                m_curSCSendMap[sigPtr->m_devCode] = *newFault;
                if(msg_index == m_curSCMsgSendMap.end())
                {
                    m_curSCMsgSendMap[sigPtr->m_areaID] = *sigPtr;
                    SYS_LOG_MESSAGE(m_logger, ll_debug, "台区第一个故障 台区 ："<< sigPtr->m_areaID << "code ："<<sigPtr->m_devCode);
                }
            }
            else
            {
                m_curSCSendMap[sigPtr->m_devCode].m_ftypeDuration = m_curSCSendMap[sigPtr->m_devCode].m_ftypeDuration + 1;
                m_curSCSendMap[sigPtr->m_devCode].m_putStoragetTime = sigPtr->m_occurDevTime;
                m_curSCSendMap[sigPtr->m_devCode].m_currenta = ((m_curSCSendMap[sigPtr->m_devCode].m_currenta + sigPtr->m_value[0])/2);
                m_curSCSendMap[sigPtr->m_devCode].m_ftypeID = sigPtr->m_faultType;
                SYS_LOG_MESSAGE(m_logger, ll_debug, "装置 ："<< sigPtr->m_devCode << "故障类型："<< sigPtr->m_faultType << "故障上发次数："<<  m_curSCSendMap[sigPtr->m_devCode].m_ftypeDuration);
            }
            
        }
    }
    else
    {
        putNewFault(sigPtr,newFault);
        newFault->m_ftypeDuration = 1;
        m_curSCSendMap[sigPtr->m_devCode] = *newFault;
            if(msg_index == m_curSCMsgSendMap.end())
            {
                m_curSCMsgSendMap[sigPtr->m_areaID] = *sigPtr;
                SYS_LOG_MESSAGE(m_logger, ll_debug, "台区第一个故障 台区 ："<< sigPtr->m_areaID << "code："<<sigPtr->m_devCode);
            }
    }
    return hr;
}



SYS_HRESULT AppDScadaTinyWorker::getDeviceStatus(SysString & deviceCode,SysString & statusValue)
{
    SYS_HRESULT hr = SYS_OK;
        std::vector<string> vec;
    SysString DeviceStatus = "SELECT DEVICE_STATUS FROM DIM_DEVICE WHERE DEVICE_CODE =  " + deviceCode;
    vec.push_back(DeviceStatus);
    SysDBModelSrvRsp rsp; 
    if (SYS_OK == (hr = m_pModelDB->exeQuery(vec, rsp)))
    {
        statusValue = rsp.m_resultsVec[0].m_recordVec[0].m_record[0].m_dataValue.valueCharPtr;
    }
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::getLineAndPole(AppDScadaSignal* sigPtr)
{
    SYS_HRESULT hr = SYS_OK;
    std::vector<string> vec;
    SysString lineAndPole = "SELECT LINE_ID ,LINE_NAME , POLE , TRANS_NAME FROM DIM_DEVICE WHERE DEVICE_CODE =  " + sigPtr->m_devCode;
    vec.push_back(lineAndPole);
    SysDBModelSrvRsp rsp; 
    if (SYS_OK == (hr = m_pModelDB->exeQuery(vec, rsp)))
    {
        SysInt i = rsp.m_resultsVec.size();
        if(rsp.m_resultsVec.size()> 0)
        {
            sigPtr->m_lineID = rsp.m_resultsVec[0].m_recordVec[0].m_record[0].m_dataValue.valueCharPtr;
            sigPtr->m_lineName = rsp.m_resultsVec[0].m_recordVec[0].m_record[1].m_dataValue.valueCharPtr;
            sigPtr->m_pole = rsp.m_resultsVec[0].m_recordVec[0].m_record[2].m_dataValue.valueCharPtr;
            sigPtr->m_areaName =  rsp.m_resultsVec[0].m_recordVec[0].m_record[3].m_dataValue.valueCharPtr;
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::getMsgDatabase(SysString & recInID)
{
    SYS_HRESULT hr = SYS_OK;
    std::vector<string> lostVoltageVec;
    std::vector<string> OverloadVec;
    SysDBModelSrvRsp rspMsg; 
    std::vector<SysString> vmsg;
    SysString msg = "SELECT MAX(REC_IN_DATE) FROM FACT_FAULT_SHORTMSG WHERE FTYPE_ID = 60";
    SysString getLostVoltageSql = "SELECT TRANS_ID,FTYPE_ID,MAX(REC_IN_DATE) FROM FACT_FAULT_SHORTMSG WHERE REC_IN_DATE > " + recInID + " AND FTYPE_ID IN('54') GROUP BY FTYPE_ID,TRANS_ID";
    SysString getOverloadSql = "SELECT DISTINCT TRANS_ID,FTYPE_ID,REC_IN_DATE FROM FACT_FAULT_SHORTMSG WHERE REC_IN_DATE IN (SELECT MAX(REC_IN_DATE) FROM FACT_FAULT_SHORTMSG\
        WHERE REC_IN_DATE > " + recInID + " AND FTYPE_ID IN('23','24') GROUP BY TRANS_ID)";
    lostVoltageVec.push_back(getLostVoltageSql);
    OverloadVec.push_back(getOverloadSql);
    vmsg.push_back(msg);
    SysDBModelSrvRsp lostVoltageRsp; 
    SysDBModelSrvRsp OverloadRsp;
    SysULong putStoragetTime = 0;
    if(SYS_OK == (hr = m_pModelDB->exeQuery(vmsg, rspMsg)))
    {
        SysString sendOutData = rspMsg.m_resultsVec[0].m_recordVec[0].m_record[0].m_dataValue.valueCharPtr;
        sscanf(sendOutData.c_str(),"%lld",&putStoragetTime);
        AppDScadaTinyWorker::m_updateDataTime = putStoragetTime;
    }

    if (SYS_OK == (hr = m_pModelDB->exeQuery(lostVoltageVec, lostVoltageRsp)))
    {

            for(SysInt i = 0; i< lostVoltageRsp.m_resultsVec[0].m_recordVec.size();i++)
            {
                SysString areaID = lostVoltageRsp.m_resultsVec[0].m_recordVec[i].m_record[0].m_dataValue.valueCharPtr;
                SysString ftypeIDs  = lostVoltageRsp.m_resultsVec[0].m_recordVec[i].m_record[1].m_dataValue.valueCharPtr;
                SysString putStoragetTimes = lostVoltageRsp.m_resultsVec[0].m_recordVec[i].m_record[2].m_dataValue.valueCharPtr;
                SysULong putStoragetTime;
                sscanf(putStoragetTimes.c_str(),"%lld",&putStoragetTime);
                SysInt ftypeID = atoi(ftypeIDs.c_str());
                m_curLostVoltageMsgMap[areaID].m_transID = areaID;
                m_curLostVoltageMsgMap[areaID].m_putStoragetTime = putStoragetTime;
                m_curLostVoltageMsgMap[areaID].m_ftypeID = ftypeID;
            }
    }
    if (SYS_OK == (hr = m_pModelDB->exeQuery(OverloadVec, OverloadRsp)))
    {

        for(SysInt i = 0; i< OverloadRsp.m_resultsVec[0].m_recordVec.size();i++)
        {
            SysString areaID = OverloadRsp.m_resultsVec[0].m_recordVec[i].m_record[0].m_dataValue.valueCharPtr;
            SysString ftypeIDs  = OverloadRsp.m_resultsVec[0].m_recordVec[i].m_record[1].m_dataValue.valueCharPtr;
            SysString putStoragetTimes = OverloadRsp.m_resultsVec[0].m_recordVec[i].m_record[2].m_dataValue.valueCharPtr;
            SysULong putStoragetTime;
            sscanf(putStoragetTimes.c_str(),"%lld",&putStoragetTime);
            SysInt ftypeID = atoi(ftypeIDs.c_str());
            m_curSCMsgMap[areaID].m_transID = areaID;
            m_curSCMsgMap[areaID].m_putStoragetTime = putStoragetTime;
            m_curSCMsgMap[areaID].m_ftypeID = ftypeID;
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::refreshDataAndReprotMsg(AppDScadaSignal* sigPtr)
{
    SYS_LOG_MESSAGE(m_logger, ll_debug, "设备："<< sigPtr->m_devCode << "故障类型"<< sigPtr->m_faultType);
    SYS_HRESULT hr = SYS_OK;
    std::vector<SysString> userNameVec;
    std::vector<SysString> phoneNumVec;
    std::vector<SysString> warnTypeVec;
    SysString gpsinfo ,faultRang ,faultName ,phaseName,devTime,faultRang_install ,devCode,sfaultNum,faultRangPole;
    SysFloat currenta = 0;

    if(sigPtr->m_faultType == AppTinyFault::LOST_VOLTAGE)
    {
        getLineAndPole(sigPtr);
    }
    SysChar faultType[15];
    sprintf(faultType,"%d",sigPtr->m_faultType);
    SysString sFaultType = faultType;
    m_pModelDB->getLineRelatedUserInfo(sigPtr->m_lineID,sFaultType,userNameVec,phoneNumVec,warnTypeVec);
    
   
    SysString m_occurDevTime = SysOSUtils::transEpochToTime(sigPtr->m_occurDevTime);
    if (sigPtr->isSCFault())
    {
        for(SysInt i = 0;i < m_curSCSendMap[sigPtr->m_devCode].devCode.size();i++)
        {
            SysString gps;
            m_pModelDB->getGpsinfo(m_curSCSendMap[sigPtr->m_devCode].devCode[i],gps);
            SysChar cCurrenta[15];
            sprintf(cCurrenta,"%0.2f",m_curSCSendMap[sigPtr->m_devCode].currenta[i]);
            SysString sCurrenta = cCurrenta;
            currenta = currenta + m_curSCSendMap[sigPtr->m_devCode].currenta[i];
            getFaultPhaseStr(m_curSCSendMap[sigPtr->m_devCode].phase[i],phaseName);
            gpsinfo = gpsinfo + "d=" + m_curSCSendMap[sigPtr->m_devCode].devCode[i] + "&" + gps + "&z=" + sigPtr->m_lineName + m_curSCSendMap[sigPtr->m_devCode].pole[i] + "杆" + phaseName + sCurrenta + "A";
            SysChar cDevTime[15];
            sprintf(cDevTime,"%lld",m_curSCSendMap[sigPtr->m_devCode].devTime[i]);
            SysString sDevTime = cDevTime;
            devTime = devTime + sDevTime;
            devCode = devCode + m_curSCSendMap[sigPtr->m_devCode].devCode[i];

            if(i <(m_curSCSendMap[sigPtr->m_devCode].devCode.size() - 1))
            {
                devTime = devTime + "#";
                devCode = devCode + ",";
                gpsinfo = gpsinfo + "@";
            }
        }
        currenta = currenta/m_curSCSendMap[sigPtr->m_devCode].devCode.size();
        SysChar cCurrenta[15];
        sprintf(cCurrenta,"%0.2f",currenta);
        SysString sCurrenta = cCurrenta;
        sfaultNum = sCurrenta + "A";
        if(sigPtr->m_faultType == 24)
        {
            faultRang = m_occurDevTime + "在" + sigPtr->m_lineName + sigPtr->m_areaName + "检查到过载情况，过载电流达:" + sCurrenta +"A。请您关注。同一台区过载情况24小时之内将不重复提示。  ";
            faultName = "过载";
        }
        else if(sigPtr->m_faultType == 23)
        {
            faultRang = m_occurDevTime + "在" + sigPtr->m_lineName + sigPtr->m_areaName + "检查到严重过载情况，过载电流达:" + sCurrenta +"A。请您关注。同一台区过载情况12小时之内将不重复提示。  ";
            faultName = "严重过载";
        }
        sigPtr ->m_devCode = m_curSCSendMap[sigPtr->m_devCode].devCode[0];
    }
    else if(sigPtr->m_faultType == AppTinyFault::LOST_VOLTAGE)
    {
        SysInt DataOrigin = 0;
        faultRang = sigPtr->m_lineName + sigPtr->m_areaName + "于" + m_occurDevTime + "发生失压故障，故障点：";//+ sigPtr->m_pole + "杆。" ;
        faultName = "失压";
        for(SysInt i = 0;i < m_curLostVoltageMsgMap[sigPtr->m_areaID].devCode.size();i++)
        {
            SysString gps;
            m_pModelDB->getGpsinfo(m_curLostVoltageMsgMap[sigPtr->m_areaID].devCode[i],gps);
            SysChar cCurrenta[15];
            sprintf(cCurrenta,"%0.2f",m_curLostVoltageMsgMap[sigPtr->m_areaID].currenta[i]);
            SysString sCurrenta = cCurrenta;
            SysChar cDevTime[15];
            sprintf(cDevTime,"%lld",m_curLostVoltageMsgMap[sigPtr->m_areaID].devTime[i]);
            SysString sDevTime = cDevTime;
            if(m_curLostVoltageMsgMap[sigPtr->m_areaID].devTime[i] != 888888)
            {
                DataOrigin = 2;
            }
            getFaultPhaseStr(m_curLostVoltageMsgMap[sigPtr->m_areaID].phase[i],phaseName);
            gpsinfo = gpsinfo + "d=" + m_curLostVoltageMsgMap[sigPtr->m_areaID].devCode[i] + "&" + gps + "&z=" + sigPtr->m_lineName + m_curLostVoltageMsgMap[sigPtr->m_areaID].pole[i] + "杆" + phaseName + sCurrenta + "V";
            devTime = devTime + sDevTime;
            devCode = devCode + m_curLostVoltageMsgMap[sigPtr->m_areaID].devCode[i];
            if(m_curLostVoltageMsgMap[sigPtr->m_areaID].install[i] == 2)
            {
                faultRang_install = sigPtr->m_lineName + sigPtr->m_areaName + "于" + m_occurDevTime + "发生失压故障，故障点：" + m_curLostVoltageMsgMap[sigPtr->m_areaID].pole[i] + "杆。";
            }
            else if(m_curLostVoltageMsgMap[sigPtr->m_areaID].pole[i].size() > 0)
            {
                faultRang = faultRang + m_curLostVoltageMsgMap[sigPtr->m_areaID].pole[i] + "杆";
            }
            else
            {
                faultRangPole = sigPtr->m_lineName + sigPtr->m_areaName + "于" + m_occurDevTime + "发生失压故障。";
            }

            if(i <(m_curLostVoltageMsgMap[sigPtr->m_areaID].devCode.size() - 1))
            {
                gpsinfo = gpsinfo + "@";
                faultRang = faultRang + "、";
                devTime = devTime + "#";
                devCode = devCode + ",";
            }
        }
        SysFloat fCurrenta = *min_element(m_curLostVoltageMsgMap[sigPtr->m_areaID].currenta.begin(),m_curLostVoltageMsgMap[sigPtr->m_areaID].currenta.end());
        SysChar cCurrenta[15];
        sprintf(cCurrenta,"%0.2f",fCurrenta);
        SysString sCurrenta = cCurrenta;
        sfaultNum = sCurrenta + "V";
       faultRang = faultRang + "。";
       if(DataOrigin == 0)
       {
            devTime = "888888";
       }

    }
    else if(sigPtr->m_faultType == AppTinyFault::IN_COMINGT)
    {
        faultRang = sigPtr->m_lineName + sigPtr->m_areaName + "于" + m_occurDevTime + "接收到一个供电恢复信号，杆塔号：";//+ sigPtr->m_pole + "杆。" ;
        faultName = "供电恢复";
        for(SysInt i = 0;i < m_curInComingtMsgMap[sigPtr->m_areaID].devCode.size();i++)
        {
            SysString gps;
            m_pModelDB->getGpsinfo(m_curInComingtMsgMap[sigPtr->m_areaID].devCode[i],gps);
            SysChar cCurrenta[15];
            sprintf(cCurrenta,"%0.2f",m_curInComingtMsgMap[sigPtr->m_areaID].currenta[i]);
            SysString sCurrenta = cCurrenta;
            SysChar cDevTime[15];
            sprintf(cDevTime,"%lld",m_curInComingtMsgMap[sigPtr->m_areaID].devTime[i]);
            SysString sDevTime = cDevTime;
            getFaultPhaseStr(m_curInComingtMsgMap[sigPtr->m_areaID].phase[i],phaseName);
            gpsinfo = gpsinfo + "d=" + m_curInComingtMsgMap[sigPtr->m_areaID].devCode[i] + "&" + gps + "&z=" + sigPtr->m_lineName + m_curInComingtMsgMap[sigPtr->m_areaID].pole[i] + "杆" + phaseName + sCurrenta + "V";
            devTime = devTime + sDevTime;
            devCode = devCode + m_curInComingtMsgMap[sigPtr->m_areaID].devCode[i];
            faultRang = faultRang + m_curInComingtMsgMap[sigPtr->m_areaID].pole[i] + "杆";
            if(i <(m_curInComingtMsgMap[sigPtr->m_areaID].devCode.size() - 1))
            {
                gpsinfo = gpsinfo + "@";
                faultRang = faultRang + "、";
                devTime = devTime + "#";
                devCode = devCode + ",";
            }
        }
        SysFloat fCurrenta = *min_element(m_curInComingtMsgMap[sigPtr->m_areaID].currenta.begin(),m_curInComingtMsgMap[sigPtr->m_areaID].currenta.end());
        SysChar cCurrenta[15];
        sprintf(cCurrenta,"%0.2f",fCurrenta);
        SysString sCurrenta = cCurrenta;
        sfaultNum = sCurrenta + "V";
        faultRang = faultRang + "。";
    }
    AppDataPersisitPkg * persistPgPtr = new AppDataPersisitPkg;
    assert(NULL != persistPgPtr);
    std::vector<SysString> sqlVec;
    if(faultRangPole.size() > 0)
    {
        sqlVec.push_back(faultRangPole);
    }
    else if((faultRang_install.size() > 0)&&(faultRangPole.size() <= 0))
    {
        sqlVec.push_back(faultRang_install);
    }
    else if((faultRang_install.size() <= 0)&&(faultRangPole.size() <= 0))
    {
        sqlVec.push_back(faultRang);
    }
    sqlVec.push_back(gpsinfo);
    sqlVec.push_back(faultName);
    sqlVec.push_back(devCode);
    sqlVec.push_back(devTime);
    sqlVec.push_back(sfaultNum);
    sigPtr->m_occurTasTime = SysOSUtils::getTimeNowMsec();
    if(SYS_OK == (hr = AppDScadaDataToDBWorker::parseShortMsgFaultData2Sql(sigPtr,persistPgPtr,sqlVec, phoneNumVec, warnTypeVec, userNameVec)))
    {
        SYS_LOG_MESSAGE(m_logger, ll_debug, "设备："<< sigPtr->m_devCode << "内容大小：" << persistPgPtr->sqlVec.size() << "用户个数："<< userNameVec.size());
        for(SysInt i = 0;i < persistPgPtr->sqlVec.size() ;i++)
        {
            SYS_LOG_MESSAGE(m_logger, ll_debug, "入库内容"<< persistPgPtr->sqlVec[i]);
        }
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_debug, "设备："<< sigPtr->m_devCode << "入短信库失败");
    }
    AppTriggerSampleQueue::getInstance()->putQ(persistPgPtr);
    return hr;
}




SYS_HRESULT AppDScadaTinyWorker::putNewFault(AppDScadaSignal* sigPtr,AppTinyFault* newFault)
{
    SYS_HRESULT hr = SYS_OK;
    newFault->m_deviceCode = sigPtr->m_devCode;
    newFault->m_putStoragetTime = sigPtr->m_occurDevTime;
    newFault->m_updateDevTime = sigPtr->m_occurDevTime;
    newFault->m_ftypeID = sigPtr->m_faultType;
    newFault->m_transID = sigPtr->m_areaID;
    newFault->m_pole = sigPtr->m_pole;
    newFault->m_phase = sigPtr->m_phase;
    newFault->m_currenta = sigPtr->m_value[0];
    newFault->m_ftypeDuration = 0;
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::sqlit(SysString & sql ,SysString & strs ,std::vector<string> & str)
{
    SYS_HRESULT hr = SYS_OK;
    SysChar * strc = new char[strlen(sql.c_str())+1];
    strcpy(strc, sql.c_str());
    SysChar* tmpStr = strtok(strc,strs.c_str());
    while (tmpStr != NULL)
    {
        str.push_back(SysString(tmpStr));
        tmpStr = strtok(NULL, strs.c_str());
    }
    delete[] strc;
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::getTransID(AppDScadaSignal* sigPtr,SysInt *install)
{
    SYS_HRESULT hr = SYS_OK;
    std::vector<string> vec;
    SysString lineAndPole = "SELECT TRANS_ID, INSTALL_TYPE,POLE FROM DIM_DEVICE WHERE DEVICE_CODE =  " + sigPtr->m_devCode;
    vec.push_back(lineAndPole);
    SysDBModelSrvRsp rsp; 
    if (SYS_OK == (hr = m_pModelDB->exeQuery(vec, rsp)))
    {
        SysInt i = rsp.m_resultsVec.size();
        if(rsp.m_resultsVec.size()> 0)
        {
            sigPtr->m_areaID = rsp.m_resultsVec[0].m_recordVec[0].m_record[0].m_dataValue.valueCharPtr;
            SysString install_type = rsp.m_resultsVec[0].m_recordVec[0].m_record[1].m_dataValue.valueCharPtr;
            sigPtr->m_pole = rsp.m_resultsVec[0].m_recordVec[0].m_record[2].m_dataValue.valueCharPtr;
            *install = atoi(install_type.c_str());
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaTinyWorker::devNormalTest()
{
    SYS_HRESULT hr = SYS_OK;
    
     SysULong now = SysOSUtils::getTimeNowMsec();
        if((AppDScadaTinyWorker::m_updateDataTime > (now - (60*60*6*1000))) || (AppDScadaTinyWorker::m_updateDataTime > now))
        {
            return hr;
        }
        else
        {
            std::vector<SysString> vdeviceSum;
            SysDBModelSrvRsp rspdevSum; 
            SysString deviceSum = "SELECT ROUND(SUM(CASE SUBSTR(DEVICE_STATE,1,1) WHEN '1' THEN 1 ELSE 0 END)/COUNT(DEVICE_CODE),4)*100 ONLINE_RATE FROM FACT_DEVICE_520_MONTIOR";
            vdeviceSum.push_back(deviceSum);
            SYS_LOG_MESSAGE(m_logger, ll_debug, "入库时差"<< ((now - (60*60*6*1000))/1000) << "时戳"<< AppDScadaTinyWorker::m_updateDataTime);
            if(SYS_OK == (hr = m_pModelDB->exeQuery(vdeviceSum, rspdevSum)))
            {
                SysUInt fdffd = AppScadaFLConfigurationMgr::getInstance()->Min_Online_Rate;
                SysChar minOnlineRate[15];
                sprintf(minOnlineRate,"%d",AppScadaFLConfigurationMgr::getInstance()->Min_Online_Rate);
                SysString m_occurDevTime = SysOSUtils::transEpochToTime(now) + "设备在线率低于"+ minOnlineRate +"%。";
                if((rspdevSum.m_resultsVec[0].m_recordVec[0].m_record[0].m_dataValue.valueDouble) < AppScadaFLConfigurationMgr::getInstance()->Min_Online_Rate)
                {
                    AppDataPersisitPkg * persistPgPtr = new AppDataPersisitPkg;
                    assert(NULL != persistPgPtr);
                    SysChar buffer[1024] = {0};
                    sprintf(buffer,"insert into FACT_FAULT_SHORTMSG (FAULT_ID,DEVICE_CODE,FTYPE_ID, FAULT_RANGE, PHNUMBER,USER_REALNAME, DISPATCH_STATUS, SMSID_ARRAY, WECHAT_STATUS,FAULT_LENGTH, REC_IN_DATE) values (\
                                   %lld,'111111111111','60','%s','%s', '%s', 11,'2',%d, 11,%lld)",now,m_occurDevTime.c_str(),AppScadaFLConfigurationMgr::getInstance()->Administrator_Phone_Number.c_str(),AppScadaFLConfigurationMgr::getInstance()->Administrator_Name.c_str(),m_occurDevTime.size(),now);
                    persistPgPtr->sqlVec.push_back(buffer);
                    AppTriggerSampleQueue::getInstance()->putQ(persistPgPtr);
                    AppDScadaTinyWorker::m_updateDataTime = now;
                }
            }
        }
    return hr;
}
