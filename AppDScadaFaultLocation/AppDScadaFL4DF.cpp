#include    "AppDScadaFL4DF.h"
#include    "AppDScadaFLConfig.h"

SYS_CLASS_LOGGER_DEF(AppDScadaFl4DFWorker);

SYS_HRESULT AppDScadaFl4DFWorker::init()
{
    SYS_HRESULT hr = SYS_OK;
    m_queue.setMode(AppDScadaMsgQueue<AppDScadaSignal>::NONE_BLOCK_MODE); // 将队列设置为非阻塞模式
    m_pModelDB = new SysDBSrvModelClient();
    m_pModelDB->refreshMaster();
    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::run()
{
    SYS_HRESULT hr = SYS_OK;
    //static const SysChar func[] = "AppDScadaFl4DFWorker::run";
    SYS_FUNC_DEF( AppDScadaFl4DFWorker::run); 
    SYS_LOG_MESSAGE(m_logger, ll_debug, func << "AppDScadaFl4DFWorker线程启动工作.");

    SYS_DEF_QUARTZ;

    SysULong counter = 0;
	//minInterval默认1小时
    SysULong minInterval = AppDScadaDropFusePolicyMgr::getInstance()->m_minFaultInterval;
    while(1)
    {
        counter ++;
        AppDScadaSignal* signalPtr = NULL;
		//get signal from Fes
        m_queue.getQ(signalPtr);
        if (NULL != signalPtr)
        {

            processDropFuseSignal(signalPtr);
        }

        //update
        if (SYS_IS_INTERVAL(5, counter))   // 5second;
        {
            updateStatus();
        }
        SYS_SLEEP;
    }

    SYS_LOG_MESSAGE(m_logger, ll_debug, func << "AppDScadaFl4DFWorker线程退出工作.");
    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::processDropFuseSignal(AppDScadaSignal* signalPtr)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_LOG_MESSAGE(m_logger, ll_debug,  "接收数据成功");
    SYS_FUNC_DEF(AppDScadaFl4DFWorker::processDropFuseSignal);
    
    SYS_ASSERT_RETURN(NULL != signalPtr, m_logger, SYS_FAIL);
    SysULong now = SysOSUtils::getTimeNowSec();
    switch(signalPtr->m_faultType)
    {
        case TAS_SHORT_WARNING_1:
        case TAS_SHORT_WARNING_2:
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, func << "跌落保险仅检测到短路信号.");
                processDropFuseSC(signalPtr);
                break;
            }
        case TAS_DROP_FUSE_SC_OFF:
        case TAS_DROP_FUSE_OFF:
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, func << "跌落保险检测到跌落信号.开始进行单点信号分析.");
                processDropFuseOff(signalPtr);
                break;
            }
        default:
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, func << "跌落保险处理器检测到未知信号：" << signalPtr->m_faultType);
                break;;
            }
    }

    return hr;
}

SysBool AppDScadaFl4DFWorker::combineAndSplitSignal(AppDScadaDropFuseFault* preSig, AppDScadaSignal* curSig)
{
    SysBool needRemove = false;

    if (preSig && (preSig->m_sig))
    {
        SysUInt sharePhase = preSig->m_sig->m_phase & curSig->m_phase;
        // 新信号中包含已有的无跌落过流故障相别，删除旧的无跌落过流信号
        if (sharePhase)
        {
            SysUInt scPhase = preSig->m_sig->m_phase - sharePhase;
            SysUInt dropPhase = curSig->m_phase | sharePhase;
            SysString showStr;
            getFaultPhaseStr(sharePhase, showStr);
            SYS_LOG_MESSAGE(m_logger, ll_debug, "合并旧的过流无跌落相别故障到新的跌落故障中， 合并相别为: " << showStr);
            // 移除合并的故障相别，原有的无跌落过流故障相别为空时，删除旧的无跌落过流故障信号
            if (!scPhase)
            {
                delete preSig;
                preSig = NULL;
                needRemove = true;
                SYS_LOG_MESSAGE(m_logger, ll_debug, "删除旧的短路无跌落信号: " << curSig->m_signalID);
            }
            else
            {
                preSig->m_sig->m_phase = scPhase;
            }

            curSig->m_phase = dropPhase;
            curSig->m_faultType = TAS_DROP_FUSE_SC_OFF;
        }
    }

    return needRemove;
}

SYS_HRESULT AppDScadaFl4DFWorker::processDropFuseOff(AppDScadaSignal* sigPtr)
{
    SYS_HRESULT hr = SYS_OK;
    SysString statusValue;
	SysString deviceLineValue;
	SysDBModelSrvRsp getLastTimestramp;
	SysULong deviceLastFaultMsgStamp = 0;
	//设备运行状态 与设备挂牌状态
	if (SYS_OK ==getDeviceStatus(sigPtr->m_devCode, statusValue) && SYS_OK == getDeviceLineStatus(sigPtr->m_devCode, deviceLineValue))
    {
        if((statusValue == "000000")||(statusValue == "000001"))
        {
			if (deviceLineValue == "0")
			{
				SYS_FUNC_DEF(AppDScadaFl4DFWorker::processDropFuseOff);
				SysULong now = SysOSUtils::getTimeNowMsec();
				// 首先在SC map中寻找；
				std::map<SysString, AppDScadaDropFuseFault*>::iterator scit = m_ScFaultMap.find(sigPtr->m_devCode);
				if (scit != m_ScFaultMap.end())
				{
					// SC 中找到对应信号； 先接收短路信号，后接收跌落信号；
					if (combineAndSplitSignal(scit->second, sigPtr))
					{
						m_ScFaultMap.erase(scit);
					}
				}

				std::map<SysString, AppDScadaDropFuseFault*>::iterator it = m_DropFaultMap.find(sigPtr->m_devCode);
				if (it != m_DropFaultMap.end())
				{
					if (SYS_OK != getDropFuseLastFaultData(sigPtr, getLastTimestramp))
					{
						return hr;
					}
					else
					{ 
						if (getLastTimestramp.m_resultsVec.at(0).m_recordVec.size() == 0)
						{
							deviceLastFaultMsgStamp = 0;
						}
						else if (getLastTimestramp.m_resultsVec.at(0).m_recordVec.at(0).m_record.size() == 0)
						{
							deviceLastFaultMsgStamp = 0;
						}
						else
						{
							deviceLastFaultMsgStamp = (SysULong)(getLastTimestramp.m_resultsVec.at(0).m_recordVec.at(0).m_record.at(0).m_dataValue.valueDouble);
						}
					}
					//0表示未查到该故障
					if (deviceLastFaultMsgStamp==0||abs((SysLong)(now - deviceLastFaultMsgStamp)) > (AppScadaFLConfigurationMgr::getInstance()->Df_MinDupFaultInterval * 1000))
					{
						// 大于设定抑制时间，可以发送;
						SYS_LOG_MESSAGE(m_logger, ll_debug, "大于设定时间，发送故障 " << (now - (it->second->m_lastFaultStamp)));
						dispatchSignalAsWarnMsg(sigPtr);
						it->second->m_lastFaultStamp = now;
					}
					else
					{
						SYS_LOG_MESSAGE(m_logger, ll_waring, "发送告警过于频繁  抑制发送" << (now - (it->second->m_lastFaultStamp)));
						SYS_LOG_MESSAGE(m_logger, ll_waring, func << "DEV(" << sigPtr->m_devCode << "#" << sigPtr->m_devCode.c_str() << ")发送告警过于频繁,当前间隔" << AppScadaFLConfigurationMgr::getInstance()->Df_MinDupFaultInterval << "s. 抑制发送.");
						delete sigPtr;
						sigPtr = NULL;
						return hr;
					}
				}
				else
				{
					// 第一次接收到该设备的跌落信号；
					SYS_LOG_MESSAGE(m_logger, ll_debug, "第一次接收到故障 ");
					AppDScadaDropFuseFault* faultPtr = NULL;
					SYS_NEW_RETURN(faultPtr, AppDScadaDropFuseFault, m_logger);
					faultPtr->m_devCode = sigPtr->m_devCode;
					faultPtr->m_lastFaultStamp = now;
					faultPtr->m_status = AppDScadaDropFuseFault::DF_OFF;
					dispatchSignalAsWarnMsg(sigPtr);
					m_DropFaultMap[sigPtr->m_devCode] = faultPtr;
				}
				delete sigPtr;
				sigPtr = NULL;
			}
			else
			{
				SYS_LOG_MESSAGE(m_logger, ll_debug, "设备所在线路正在检修，退出此次故障判断。");
			}
        }
        else
        {
            SYS_LOG_MESSAGE(m_logger, ll_debug, "装置没有正常工作 ");
        }
    }
    return hr;
}


SYS_HRESULT AppDScadaFl4DFWorker::processDropFuseSC(AppDScadaSignal* sigPtr)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_FUNC_DEF(AppDScadaFl4DFWorker::processDropFuseSC);
    SYS_ASSERT_RETURN( sigPtr != NULL, m_logger, SYS_FAIL);


    std::map<SysString, AppDScadaDropFuseFault*>::iterator scit = m_ScFaultMap.find(sigPtr->m_devCode);
    if (scit != m_ScFaultMap.end())
    {
		//位与 判断相别 
		//111 由低向高分别代表ABC三相
        SysUInt sharePhase = scit->second->m_sig->m_phase & sigPtr->m_phase;
        SysUInt combinePhase = sigPtr->m_phase - sharePhase;
        if (combinePhase)
        {
            SysString showStr;
            getFaultPhaseStr(combinePhase, showStr);
            // 新的短路信号故障相别合并
            scit->second->m_sig->m_phase |= combinePhase;
            SYS_LOG_MESSAGE(m_logger, ll_debug, "合并新接收的短路/过流无跌落故障信号, 合并相别: " << showStr);
            // 更新时戳，重新召测相关设备负荷
            scit->second->m_lastFaultStamp = SysOSUtils::getTimeNowMsec();
            deliverCallData(scit->second->m_sig->m_devCode);
        }
        else
        {
            SYS_LOG_MESSAGE(m_logger, ll_waring, "设备:" << scit->first << "已经发出SC信号.");
        }

        delete sigPtr;
        sigPtr = NULL;
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_debug, "SC MAP 第一次收到短路SOE.");
        processDropFuseSCFirst(sigPtr);
    }

    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::deliverCallData(SysString& devCode)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_FUNC_DEF(AppDScadaFl4DFWorker::deliverCallData);

    SYS_LOG_MESSAGE(m_logger, ll_debug, func << " 发送总召请求.");

    webServiceReq * dfCallAllPkgPtr = NULL;
    SYS_NEW_RETURN(dfCallAllPkgPtr, webServiceReq, m_logger);
    dfCallAllPkgPtr->set_reqtype(webServiceProtocol::remoteReqProtocol_cmdType_CALLALL);
    dfCallAllPkgPtr->set_timestamp(SysOSUtils::getTimeNowMsec());
    dfCallAllPkgPtr->set_devcode(devCode);
    dfCallAllPkgPtr->set_pointnum(0);
    dfCallAllPkgPtr->set_servicetag("AppDScadaFaultLocation");

    AppFLWebCallDataQueue::getInstance()->putQ(dfCallAllPkgPtr);

    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::processDropFuseSCFirst(AppDScadaSignal* sigPtr)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_FUNC_DEF(AppDScadaFl4DFWorker::processDropFuseSCFirst);
    SYS_ASSERT_RETURN( sigPtr != NULL, m_logger, SYS_FAIL);
	SysString deviceLineValue;
	if (SYS_OK == getDeviceLineStatus(sigPtr->m_devCode, deviceLineValue))
	{
		if ("0" == deviceLineValue)
		{
			AppDScadaDropFuseFault* scFaultPtr = NULL;
			SYS_NEW_RETURN(scFaultPtr, AppDScadaDropFuseFault, m_logger);
			scFaultPtr->m_devCode = sigPtr->m_devCode;
			scFaultPtr->m_lastFaultStamp = SysOSUtils::getTimeNowMsec();
			scFaultPtr->m_sig = sigPtr;
			scFaultPtr->m_status = AppDScadaDropFuseFault::DF_SC_ONLY;
			m_ScFaultMap[sigPtr->m_devCode] = scFaultPtr;
			SYS_LOG_MESSAGE(m_logger, ll_debug, "策略：" << AppScadaFLConfigurationMgr::getInstance()->Df_RecvSCWaitDFOffDelay << "s 后开始检测负荷电流.");

			deliverCallData(sigPtr->m_devCode);
		}
		else
		{
			SYS_LOG_MESSAGE(m_logger,ll_waring, "设备所在线路正在检修，终止本次故障判断");
		}
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger,ll_error, "获取设备所在线路信息失败！");
	}
    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::updateStatus()
{
    SYS_HRESULT hr = SYS_OK;
    SYS_FUNC_DEF(AppDScadaFl4DFWorker::updateStatus);
    std::map<SysString, AppDScadaDropFuseFault*>::iterator scit = m_ScFaultMap.begin();
    
    m_removeSigVec.clear();
    
    SysULong now = SysOSUtils::getTimeNowMsec();
    for (; scit != m_ScFaultMap.end(); scit ++)
    {
        if (NULL != scit->second)
        {
            AppDScadaDropFuseFault* faultPtr = scit->second;
			//接收到短路信号60s后报送故障信息
            if ( abs((SysLong)(now - faultPtr->m_lastFaultStamp))  > (AppScadaFLConfigurationMgr::getInstance()->Df_RecvSCWaitDFOffDelay * 1000))
            {
                // 等待召测数据到时；开始分析数据
                analysisCallData(scit->second);
            }
        }
    }
	//从故障图中移除被标记为丢弃的故障信号
    for (int i = 0; i < m_removeSigVec.size() ; i++)
    {
        std::map<SysString, AppDScadaDropFuseFault*>::iterator scit = m_ScFaultMap.find(m_removeSigVec[i]);

        if (scit != m_ScFaultMap.end())
        {
            delete scit->second;
            m_ScFaultMap.erase(scit);
        }
    }
    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::getDropFuseDescription(SysUInt faultType, SysString& faultName ,SysString& faultID)
{
    SYS_HRESULT hr = SYS_OK;

    switch(faultType & 0x0FFFFFFF)
    {
    case TAS_SHORT_WARNING_3:
    case TAS_SHORT_WARNING_2:
    case TAS_SHORT_WARNING_1:
        {
            faultName = "故障未跌落";
            faultID = "23";
            break;
        }
    case TAS_DROP_FUSE_SC_OFF:
        {
            faultName = "故障跌落";
            faultID = "52";
            break;
        }
    case TAS_DROP_FUSE_OFF:
        {
            faultName = "异常跌落";
            faultID = "53";
            break;
        }
    default:
        {
            faultName = "未知类型故障";
            break;
        }
    }

    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::getDropFuseFaultInfo(AppDScadaSignal* sig, std::vector<SysString>& faultInfoVec)
{
    SYS_HRESULT hr = SYS_OK;

    m_modelDB.refreshMaster();

    SysString sql = "SELECT LONGITUDE_GPS, LATITUDE_GPS FROM FACT_DEVICE_DROP_MONITOR where DEVICE_CODE = " + sig->m_devCode;
    std::vector<SysString> sqlVec;
    SysDBModelSrvRsp rsp; 
    sqlVec.push_back(sql);
    if (SYS_OK != m_modelDB.exeQuery(sqlVec, rsp))
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "查询GPS信息失败! AppDScadaFl4DFWorker::getDropFuseFaultInfo()");
        return SYS_FAIL;
    }

    if (rsp.m_resultsVec.size() && SYS_OK == rsp.m_resultsVec[0].m_hr)
    {
        SysString gpsInfo, section, faultName, faultType;

        SysChar tmp[512] = {0};
        SysString longitude, latitude;
        sprintf(tmp, "%7f", rsp.m_resultsVec[0].m_recordVec[0].m_record[0].m_dataValue.valueDouble);
        longitude = tmp;
        memset(tmp, 0, sizeof(tmp));
        sprintf(tmp, "%7f", rsp.m_resultsVec[0].m_recordVec[0].m_record[1].m_dataValue.valueDouble);
        latitude = tmp;
        //gpsInfo = "x=" + longitude + "&y=" + latitude + "&z=" + sig->m_lineName + sig->m_pole + "杆" ;
        SysString occurTime = SysOSUtils::transEpochToTime(sig->m_occurDevTime);
        getDropFuseDescription(sig->m_faultType, faultName, faultType);
        SysString phaseName;
        getFaultPhaseStr(sig->m_phase, phaseName);
        gpsInfo = "d=" +sig->m_devCode + "&" +  "x=" + longitude + "&y=" + latitude + "&z=" + sig->m_lineName + sig->m_pole + "杆" + phaseName ;
        section = sig->m_lineName + sig->m_pole + "杆" + "跌落保险于"+ occurTime+"发生" + faultName + "，相别:" + phaseName +"。";

        SysString signalStr;
        SysOSUtils::numberToString(sig->m_signalID, signalStr);

        faultInfoVec.push_back(section);
        faultInfoVec.push_back(gpsInfo);
        faultInfoVec.push_back(faultName);
        faultInfoVec.push_back(signalStr);
        faultInfoVec.push_back(faultType);
        faultInfoVec.push_back(longitude);
        faultInfoVec.push_back(latitude);
    }

    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::getDropFuseRtData(AppDScadaDropFuseFault* faultPtr, SysDBModelSrvRsp& rsp)
{
    SYS_HRESULT hr = SYS_OK;

    m_modelDB.refreshMaster();

    std::vector<SysString> sqlVec;
    SysString sql = "SELECT IA, IB, IC, TIMESTAMP FROM FACT_DEVICE_DROP_MONITOR WHERE DEVICE_CODE = " + faultPtr->m_devCode;
    sqlVec.push_back(sql);
    if (SYS_OK != m_modelDB.exeQuery(sqlVec, rsp))
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "查询跌落保险实时数据失败! AppDScadaFl4DFWorker::getDropFuseRtData()");
        return SYS_FAIL;
    }

    if ((SYS_OK != rsp.m_resultsVec[0].m_hr) || (!rsp.m_resultsVec.size()) || (rsp.m_resultsVec[0].m_recordVec[0].m_record.size() != 4))
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "查询跌落保险实时数据有误! AppDScadaFl4DFWorker::getDropFuseRtData()");
        return SYS_FAIL;
    }

    return hr;
}
SYS_HRESULT AppDScadaFl4DFWorker::getDropFuseLastFaultData(AppDScadaSignal * faultPtr, SysDBModelSrvRsp & rsp)
{

	SYS_HRESULT hr = SYS_OK;
	m_modelDB.refreshMaster();
	std::vector<SysString> sqlVec;
	SysString sql = "SELECT \
		MAX(TO_DATE(TO_CHAR(UPDATE_DATE, 'yyyy-mm-dd hh24:mi:ss'), 'yyyy-mm-dd hh24:mi:ss') - TO_DATE('1970-01-01 08:00:00', 'yyyy-mm-dd hh24:mi:ss')) * 1000 * 24 * 3600 \
		FROM FACT_FAULT_SHORTMSG\
		WHERE DEVICE_CODE ="+ faultPtr->m_devCode+"AND FTYPE_ID =" \
		+std::to_string(static_cast<long long>(faultPtr->m_faultType))+\
		"AND PHASE ="+std::to_string(static_cast<long long>(faultPtr->m_phase));
	sqlVec.push_back(sql);
	if (SYS_OK != m_modelDB.exeQuery(sqlVec, rsp))
	{
		SYS_LOG_MESSAGE(m_logger, ll_error, "查询短信表历史故障数据失败！AppDScadaFl4DFWorker::getDropFuseLastFaultData()");
		return SYS_FAIL;
	}
	if ((SYS_OK != rsp.m_resultsVec[0].m_hr) || (!rsp.m_resultsVec.size()))
	{
		SYS_LOG_MESSAGE(m_logger, ll_error, "查询短信表历史故障数据有误! AppDScadaFl4DFWorker::getDropFuseRtData()");
		return SYS_FAIL;
	}
	return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::analysisCallData(AppDScadaDropFuseFault* faultPtr)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_FUNC_DEF(AppDScadaFl4DFWorker::analysisCallData());
    SYS_ASSERT_RETURN(faultPtr != NULL, m_logger, SYS_FAIL);
    SYS_ASSERT_RETURN(NULL != faultPtr->m_sig, m_logger, SYS_FAIL);

    SysULong now = SysOSUtils::getTimeNowMsec();
    SysDBModelSrvRsp rsp;
	SysDBModelSrvRsp getLastTimestramp;
	SysULong deviceLastFaultMsgStamp = 0;
    if (SYS_OK != getDropFuseRtData(faultPtr, rsp))
    {
        return hr;
    }
	else if (SYS_OK != getDropFuseLastFaultData(faultPtr->m_sig, getLastTimestramp))
	{
		return hr;
	}
    else
    {
        SysFloat ia = (SysFloat)(rsp.m_resultsVec[0].m_recordVec[0].m_record[0].m_dataValue.valueDouble);
        SysFloat ib = (SysFloat)(rsp.m_resultsVec[0].m_recordVec[0].m_record[1].m_dataValue.valueDouble);
        SysFloat ic = (SysFloat)(rsp.m_resultsVec[0].m_recordVec[0].m_record[2].m_dataValue.valueDouble);
        SysULong deviceUpdateStamp = (SysULong)(rsp.m_resultsVec[0].m_recordVec[0].m_record[3].m_dataValue.valueDouble);
		//该设备上次同相别同故障类型的故障时间 查不到默认为0
		if (getLastTimestramp.m_resultsVec.at(0).m_recordVec.size() == 0)
		{
			deviceLastFaultMsgStamp = 0;
		}
		else if(getLastTimestramp.m_resultsVec.at(0).m_recordVec.at(0).m_record.size() == 0)
		{
			deviceLastFaultMsgStamp = 0;
		}
		else
		{
			deviceLastFaultMsgStamp= (SysULong)(getLastTimestramp.m_resultsVec.at(0).m_recordVec.at(0).m_record.at(0).m_dataValue.valueDouble);
		}
		
        if (deviceUpdateStamp > faultPtr->m_lastFaultStamp)
        {
            // 数据已经刷新
            SYS_LOG_MESSAGE(m_logger, ll_waring, "检测到数据刷新. 开始判定负荷变化");
            if (overloadIsChanged(ia, ib, ic))
            {
                // 负荷发生突变降低，报送故障信息；
                SYS_LOG_MESSAGE(m_logger, ll_debug, "负荷突变,线路发生故障，但是跌落保险可能未跌落");
				if (deviceLastFaultMsgStamp==0||abs(SysLong(deviceUpdateStamp - deviceLastFaultMsgStamp)) > AppScadaFLConfigurationMgr::getInstance()->Df_MinDupFaultInterval * 1000)
				{
					SYS_LOG_MESSAGE(m_logger, ll_debug, "故障间隔大于故障抑制时间，发送故障"<<"时间戳："<<deviceLastFaultMsgStamp);
					dispatchSignalAsWarnMsg(faultPtr->m_sig);
					m_removeSigVec.push_back(faultPtr->m_devCode);
				}
				else
				{
					SYS_LOG_MESSAGE(m_logger, ll_debug, "发送告警过于频繁，抑制发送" << (deviceUpdateStamp - deviceLastFaultMsgStamp)/1000<<"s");
				}
            }
            else
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, func << " 接收到短路信号，但是线路还存在电流." << ia << "A " << ib << "A " << ic << "A");
                // 认为短路信号是一个虚假信号，不予处理；
                m_removeSigVec.push_back(faultPtr->m_devCode);
            }

        }
        else
        {
            //数据未刷新;
            SYS_LOG_MESSAGE(m_logger, ll_waring, func << "数据未刷新。。");
			//召测次数大于2次认定召测失败，屏蔽信号
            if (faultPtr->m_counter > 2)
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, "召测数据未返回，屏蔽SC信号.DEV:" << faultPtr->m_devCode);
                m_removeSigVec.push_back(faultPtr->m_devCode);
                return hr;
            }
			//召测次数小于2次，重新召测计数器+1，更新时间戳
            else
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, "等待" << AppScadaFLConfigurationMgr::getInstance()->Df_RecvSCWaitDFOffDelay << "s 后未有数据返回.再次发送召测请求.");
                faultPtr->m_counter ++;
                deliverCallData(faultPtr->m_sig->m_devCode);
                faultPtr->m_lastFaultStamp = now;
            }
        }
    }

    return hr;
}

SysBool     AppDScadaFl4DFWorker::overloadIsChanged(SysFloat ia, SysFloat ib, SysFloat ic)
{
    return (OVERLOAD_IS_ZERO(ia) || OVERLOAD_IS_ZERO(ib) || OVERLOAD_IS_ZERO(ic));
}

SYS_HRESULT AppDScadaFl4DFWorker::dispatchSignalAsWarnMsg(AppDScadaSignal* sig)
{
    SYS_HRESULT hr = SYS_OK;
    if (SYS_OK != m_modelDB.refreshMaster())
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "连接数据库失败! AppDScadaFl4DFWorker::dispatchSignalAsWarnMsg()");
        return SYS_FAIL;
    }

    std::vector<SysString> faultInfoVec; // 0:section 1:gps 2:faultName 3:signalStr 4:faultType
    hr = getDropFuseFaultInfo(sig, faultInfoVec);
    if (SYS_OK != hr || faultInfoVec.size() != 7)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "故障信息查询失败! AppDScadaFl4DFWorker::dispatchSignalAsWarnMsg()");
        return SYS_FAIL;
    }

    std::vector<SysString> userNameVec;
    std::vector<SysString> phoneNumVec;
    std::vector<SysString> warnTypeVec;
    if (SYS_OK != m_modelDB.getLineRelatedUserInfo(sig->m_lineID, faultInfoVec[FAULT_INFO_TYPE], userNameVec, phoneNumVec, warnTypeVec))
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "查询故障订阅人员信息失败! AppDScadaFl4DFWorker::dispatchSignalAsWarnMsg()");
        return SYS_FAIL;
    }

    AppDataPersisitPkg * persistPgPtr = new AppDataPersisitPkg;
    assert(NULL != persistPgPtr);
    
    SysChar tmp[512] = {0};
    sprintf(tmp, "%7f", sig->m_value[0]);
    SysString currenta = tmp;
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "%lld", sig->m_occurDevTime);
    SysString occurDevTime = tmp;
    SysString m_occurDevTime = SysOSUtils::transEpochToTime(sig->m_occurDevTime);

    SysString updateSwitchStatus_M = "UPDATE FACT_DEVICE_DROP_MONITOR SET SWITCH_STATUS = 2 , IA = "+ currenta + " , IB = " + currenta + ", IC = " +  currenta + ",UPDATE_DATE = to_date('" + m_occurDevTime + "','yyyy-mm-dd hh24:mi:ss'),TIMESTAMP = " + occurDevTime + " WHERE DEVICE_CODE = " + sig->m_devCode;
    SysString updateSwitchStatus_H = "insert into FACT_DEVICE_DROP_HISTORY (DEVICE_CODE, UPDATE_DATE, IA , IB, IC, SWITCH_STATUS, LONGITUDE_GPS,LATITUDE_GPS, TIMESTAMP, DEVICE_TYPE) values ('" + sig->m_devCode +"',to_date('" + m_occurDevTime + "','yyyy-mm-dd hh24:mi:ss'),\
        " + currenta + "," + currenta + "," + currenta + ",2," + faultInfoVec[LONGITUDE_GPS] + "," + faultInfoVec[LATITUDE_GPS] + "," + occurDevTime + ",11)";
    persistPgPtr->sqlVec.push_back(updateSwitchStatus_H);
    persistPgPtr->sqlVec.push_back(updateSwitchStatus_M);
    SysChar cDevTime[15];
    sprintf(cDevTime,"%lld",sig->m_occurDevTime);
    SysString sDevTime = cDevTime;
    SysChar cCurrenta[15];
    sprintf(cCurrenta,"%0.2f",sig->m_value[0]);
    SysString sCurrenta = cCurrenta;
    SysString sfaultNum = sCurrenta + "A";
    faultInfoVec[1]= faultInfoVec[1] + sfaultNum;
    faultInfoVec.push_back(sig->m_devCode);
    faultInfoVec.push_back(sDevTime);
    faultInfoVec.push_back(sfaultNum);
    sig->m_areaID = sig->m_lineID;
    sig->m_areaName = sig->m_lineName;
    if(SYS_OK == (hr = AppDScadaDataToDBWorker::putFaultDatabase2Sql(sig, persistPgPtr, faultInfoVec)));
    AppDScadaDataToDBWorker::parseShortMsgFaultData2Sql(sig, persistPgPtr, faultInfoVec, phoneNumVec, warnTypeVec, userNameVec);
    AppTriggerSampleQueue::getInstance()->putQ(persistPgPtr);

    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::getDeviceStatus(SysString & deviceCode,SysString & statusValue)
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
SYS_HRESULT AppDScadaFl4DFWorker::getDeviceLineStatus(SysString & deviceCode, SysString & statusValue)
{
	SYS_HRESULT hr = SYS_OK;
	std::vector<string> vec;
	SysString DeviceStatus = "SELECT HANG_STATUS FROM DIM_DEVICE WHERE DEVICE_CODE =" + deviceCode ;
	vec.push_back(DeviceStatus);
	SysDBModelSrvRsp rsp;
	if (SYS_OK == (hr = m_pModelDB->exeQuery(vec, rsp)))
	{
		statusValue = rsp.m_resultsVec[0].m_recordVec[0].m_record[0].m_dataValue.valueCharPtr;
	}
	return hr;
}