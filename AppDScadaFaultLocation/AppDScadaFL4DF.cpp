#include    "AppDScadaFL4DF.h"
#include    "AppDScadaFLConfig.h"

SYS_CLASS_LOGGER_DEF(AppDScadaFl4DFWorker);

SYS_HRESULT AppDScadaFl4DFWorker::init()
{
    SYS_HRESULT hr = SYS_OK;
    m_queue.setMode(AppDScadaMsgQueue<AppDScadaSignal>::NONE_BLOCK_MODE); // ����������Ϊ������ģʽ
    m_pModelDB = new SysDBSrvModelClient();
    m_pModelDB->refreshMaster();
    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::run()
{
    SYS_HRESULT hr = SYS_OK;
    //static const SysChar func[] = "AppDScadaFl4DFWorker::run";
    SYS_FUNC_DEF( AppDScadaFl4DFWorker::run); 
    SYS_LOG_MESSAGE(m_logger, ll_debug, func << "AppDScadaFl4DFWorker�߳���������.");

    SYS_DEF_QUARTZ;

    SysULong counter = 0;
	//minIntervalĬ��1Сʱ
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

    SYS_LOG_MESSAGE(m_logger, ll_debug, func << "AppDScadaFl4DFWorker�߳��˳�����.");
    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::processDropFuseSignal(AppDScadaSignal* signalPtr)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_LOG_MESSAGE(m_logger, ll_debug,  "�������ݳɹ�");
    SYS_FUNC_DEF(AppDScadaFl4DFWorker::processDropFuseSignal);
    
    SYS_ASSERT_RETURN(NULL != signalPtr, m_logger, SYS_FAIL);
    SysULong now = SysOSUtils::getTimeNowSec();
    switch(signalPtr->m_faultType)
    {
        case TAS_SHORT_WARNING_1:
        case TAS_SHORT_WARNING_2:
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, func << "���䱣�ս���⵽��·�ź�.");
                processDropFuseSC(signalPtr);
                break;
            }
        case TAS_DROP_FUSE_SC_OFF:
        case TAS_DROP_FUSE_OFF:
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, func << "���䱣�ռ�⵽�����ź�.��ʼ���е����źŷ���.");
                processDropFuseOff(signalPtr);
                break;
            }
        default:
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, func << "���䱣�մ�������⵽δ֪�źţ�" << signalPtr->m_faultType);
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
        // ���ź��а������е��޵�������������ɾ���ɵ��޵�������ź�
        if (sharePhase)
        {
            SysUInt scPhase = preSig->m_sig->m_phase - sharePhase;
            SysUInt dropPhase = curSig->m_phase | sharePhase;
            SysString showStr;
            getFaultPhaseStr(sharePhase, showStr);
            SYS_LOG_MESSAGE(m_logger, ll_debug, "�ϲ��ɵĹ����޵��������ϵ��µĵ�������У� �ϲ����Ϊ: " << showStr);
            // �Ƴ��ϲ��Ĺ������ԭ�е��޵�������������Ϊ��ʱ��ɾ���ɵ��޵�����������ź�
            if (!scPhase)
            {
                delete preSig;
                preSig = NULL;
                needRemove = true;
                SYS_LOG_MESSAGE(m_logger, ll_debug, "ɾ���ɵĶ�·�޵����ź�: " << curSig->m_signalID);
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
	//�豸����״̬ ���豸����״̬
	if (SYS_OK ==getDeviceStatus(sigPtr->m_devCode, statusValue) && SYS_OK == getDeviceLineStatus(sigPtr->m_devCode, deviceLineValue))
    {
        if((statusValue == "000000")||(statusValue == "000001"))
        {
			if (deviceLineValue == "0")
			{
				SYS_FUNC_DEF(AppDScadaFl4DFWorker::processDropFuseOff);
				SysULong now = SysOSUtils::getTimeNowMsec();
				// ������SC map��Ѱ�ң�
				std::map<SysString, AppDScadaDropFuseFault*>::iterator scit = m_ScFaultMap.find(sigPtr->m_devCode);
				if (scit != m_ScFaultMap.end())
				{
					// SC ���ҵ���Ӧ�źţ� �Ƚ��ն�·�źţ�����յ����źţ�
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
					//0��ʾδ�鵽�ù���
					if (deviceLastFaultMsgStamp==0||abs((SysLong)(now - deviceLastFaultMsgStamp)) > (AppScadaFLConfigurationMgr::getInstance()->Df_MinDupFaultInterval * 1000))
					{
						// �����趨����ʱ�䣬���Է���;
						SYS_LOG_MESSAGE(m_logger, ll_debug, "�����趨ʱ�䣬���͹��� " << (now - (it->second->m_lastFaultStamp)));
						dispatchSignalAsWarnMsg(sigPtr);
						it->second->m_lastFaultStamp = now;
					}
					else
					{
						SYS_LOG_MESSAGE(m_logger, ll_waring, "���͸澯����Ƶ��  ���Ʒ���" << (now - (it->second->m_lastFaultStamp)));
						SYS_LOG_MESSAGE(m_logger, ll_waring, func << "DEV(" << sigPtr->m_devCode << "#" << sigPtr->m_devCode.c_str() << ")���͸澯����Ƶ��,��ǰ���" << AppScadaFLConfigurationMgr::getInstance()->Df_MinDupFaultInterval << "s. ���Ʒ���.");
						delete sigPtr;
						sigPtr = NULL;
						return hr;
					}
				}
				else
				{
					// ��һ�ν��յ����豸�ĵ����źţ�
					SYS_LOG_MESSAGE(m_logger, ll_debug, "��һ�ν��յ����� ");
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
				SYS_LOG_MESSAGE(m_logger, ll_debug, "�豸������·���ڼ��ޣ��˳��˴ι����жϡ�");
			}
        }
        else
        {
            SYS_LOG_MESSAGE(m_logger, ll_debug, "װ��û���������� ");
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
		//λ�� �ж���� 
		//111 �ɵ���߷ֱ����ABC����
        SysUInt sharePhase = scit->second->m_sig->m_phase & sigPtr->m_phase;
        SysUInt combinePhase = sigPtr->m_phase - sharePhase;
        if (combinePhase)
        {
            SysString showStr;
            getFaultPhaseStr(combinePhase, showStr);
            // �µĶ�·�źŹ������ϲ�
            scit->second->m_sig->m_phase |= combinePhase;
            SYS_LOG_MESSAGE(m_logger, ll_debug, "�ϲ��½��յĶ�·/�����޵�������ź�, �ϲ����: " << showStr);
            // ����ʱ���������ٲ�����豸����
            scit->second->m_lastFaultStamp = SysOSUtils::getTimeNowMsec();
            deliverCallData(scit->second->m_sig->m_devCode);
        }
        else
        {
            SYS_LOG_MESSAGE(m_logger, ll_waring, "�豸:" << scit->first << "�Ѿ�����SC�ź�.");
        }

        delete sigPtr;
        sigPtr = NULL;
    }
    else
    {
        SYS_LOG_MESSAGE(m_logger, ll_debug, "SC MAP ��һ���յ���·SOE.");
        processDropFuseSCFirst(sigPtr);
    }

    return hr;
}

SYS_HRESULT AppDScadaFl4DFWorker::deliverCallData(SysString& devCode)
{
    SYS_HRESULT hr = SYS_OK;
    SYS_FUNC_DEF(AppDScadaFl4DFWorker::deliverCallData);

    SYS_LOG_MESSAGE(m_logger, ll_debug, func << " ������������.");

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
			SYS_LOG_MESSAGE(m_logger, ll_debug, "���ԣ�" << AppScadaFLConfigurationMgr::getInstance()->Df_RecvSCWaitDFOffDelay << "s ��ʼ��⸺�ɵ���.");

			deliverCallData(sigPtr->m_devCode);
		}
		else
		{
			SYS_LOG_MESSAGE(m_logger,ll_waring, "�豸������·���ڼ��ޣ���ֹ���ι����ж�");
		}
	}
	else
	{
		SYS_LOG_MESSAGE(m_logger,ll_error, "��ȡ�豸������·��Ϣʧ�ܣ�");
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
			//���յ���·�ź�60s���͹�����Ϣ
            if ( abs((SysLong)(now - faultPtr->m_lastFaultStamp))  > (AppScadaFLConfigurationMgr::getInstance()->Df_RecvSCWaitDFOffDelay * 1000))
            {
                // �ȴ��ٲ����ݵ�ʱ����ʼ��������
                analysisCallData(scit->second);
            }
        }
    }
	//�ӹ���ͼ���Ƴ������Ϊ�����Ĺ����ź�
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
            faultName = "����δ����";
            faultID = "23";
            break;
        }
    case TAS_DROP_FUSE_SC_OFF:
        {
            faultName = "���ϵ���";
            faultID = "52";
            break;
        }
    case TAS_DROP_FUSE_OFF:
        {
            faultName = "�쳣����";
            faultID = "53";
            break;
        }
    default:
        {
            faultName = "δ֪���͹���";
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
        SYS_LOG_MESSAGE(m_logger, ll_error, "��ѯGPS��Ϣʧ��! AppDScadaFl4DFWorker::getDropFuseFaultInfo()");
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
        //gpsInfo = "x=" + longitude + "&y=" + latitude + "&z=" + sig->m_lineName + sig->m_pole + "��" ;
        SysString occurTime = SysOSUtils::transEpochToTime(sig->m_occurDevTime);
        getDropFuseDescription(sig->m_faultType, faultName, faultType);
        SysString phaseName;
        getFaultPhaseStr(sig->m_phase, phaseName);
        gpsInfo = "d=" +sig->m_devCode + "&" +  "x=" + longitude + "&y=" + latitude + "&z=" + sig->m_lineName + sig->m_pole + "��" + phaseName ;
        section = sig->m_lineName + sig->m_pole + "��" + "���䱣����"+ occurTime+"����" + faultName + "�����:" + phaseName +"��";

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
        SYS_LOG_MESSAGE(m_logger, ll_error, "��ѯ���䱣��ʵʱ����ʧ��! AppDScadaFl4DFWorker::getDropFuseRtData()");
        return SYS_FAIL;
    }

    if ((SYS_OK != rsp.m_resultsVec[0].m_hr) || (!rsp.m_resultsVec.size()) || (rsp.m_resultsVec[0].m_recordVec[0].m_record.size() != 4))
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "��ѯ���䱣��ʵʱ��������! AppDScadaFl4DFWorker::getDropFuseRtData()");
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
		SYS_LOG_MESSAGE(m_logger, ll_error, "��ѯ���ű���ʷ��������ʧ�ܣ�AppDScadaFl4DFWorker::getDropFuseLastFaultData()");
		return SYS_FAIL;
	}
	if ((SYS_OK != rsp.m_resultsVec[0].m_hr) || (!rsp.m_resultsVec.size()))
	{
		SYS_LOG_MESSAGE(m_logger, ll_error, "��ѯ���ű���ʷ������������! AppDScadaFl4DFWorker::getDropFuseRtData()");
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
		//���豸�ϴ�ͬ���ͬ�������͵Ĺ���ʱ�� �鲻��Ĭ��Ϊ0
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
            // �����Ѿ�ˢ��
            SYS_LOG_MESSAGE(m_logger, ll_waring, "��⵽����ˢ��. ��ʼ�ж����ɱ仯");
            if (overloadIsChanged(ia, ib, ic))
            {
                // ���ɷ���ͻ�併�ͣ����͹�����Ϣ��
                SYS_LOG_MESSAGE(m_logger, ll_debug, "����ͻ��,��·�������ϣ����ǵ��䱣�տ���δ����");
				if (deviceLastFaultMsgStamp==0||abs(SysLong(deviceUpdateStamp - deviceLastFaultMsgStamp)) > AppScadaFLConfigurationMgr::getInstance()->Df_MinDupFaultInterval * 1000)
				{
					SYS_LOG_MESSAGE(m_logger, ll_debug, "���ϼ�����ڹ�������ʱ�䣬���͹���"<<"ʱ�����"<<deviceLastFaultMsgStamp);
					dispatchSignalAsWarnMsg(faultPtr->m_sig);
					m_removeSigVec.push_back(faultPtr->m_devCode);
				}
				else
				{
					SYS_LOG_MESSAGE(m_logger, ll_debug, "���͸澯����Ƶ�������Ʒ���" << (deviceUpdateStamp - deviceLastFaultMsgStamp)/1000<<"s");
				}
            }
            else
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, func << " ���յ���·�źţ�������·�����ڵ���." << ia << "A " << ib << "A " << ic << "A");
                // ��Ϊ��·�ź���һ������źţ����账��
                m_removeSigVec.push_back(faultPtr->m_devCode);
            }

        }
        else
        {
            //����δˢ��;
            SYS_LOG_MESSAGE(m_logger, ll_waring, func << "����δˢ�¡���");
			//�ٲ��������2���϶��ٲ�ʧ�ܣ������ź�
            if (faultPtr->m_counter > 2)
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, "�ٲ�����δ���أ�����SC�ź�.DEV:" << faultPtr->m_devCode);
                m_removeSigVec.push_back(faultPtr->m_devCode);
                return hr;
            }
			//�ٲ����С��2�Σ������ٲ������+1������ʱ���
            else
            {
                SYS_LOG_MESSAGE(m_logger, ll_waring, "�ȴ�" << AppScadaFLConfigurationMgr::getInstance()->Df_RecvSCWaitDFOffDelay << "s ��δ�����ݷ���.�ٴη����ٲ�����.");
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
        SYS_LOG_MESSAGE(m_logger, ll_error, "�������ݿ�ʧ��! AppDScadaFl4DFWorker::dispatchSignalAsWarnMsg()");
        return SYS_FAIL;
    }

    std::vector<SysString> faultInfoVec; // 0:section 1:gps 2:faultName 3:signalStr 4:faultType
    hr = getDropFuseFaultInfo(sig, faultInfoVec);
    if (SYS_OK != hr || faultInfoVec.size() != 7)
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "������Ϣ��ѯʧ��! AppDScadaFl4DFWorker::dispatchSignalAsWarnMsg()");
        return SYS_FAIL;
    }

    std::vector<SysString> userNameVec;
    std::vector<SysString> phoneNumVec;
    std::vector<SysString> warnTypeVec;
    if (SYS_OK != m_modelDB.getLineRelatedUserInfo(sig->m_lineID, faultInfoVec[FAULT_INFO_TYPE], userNameVec, phoneNumVec, warnTypeVec))
    {
        SYS_LOG_MESSAGE(m_logger, ll_error, "��ѯ���϶�����Ա��Ϣʧ��! AppDScadaFl4DFWorker::dispatchSignalAsWarnMsg()");
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